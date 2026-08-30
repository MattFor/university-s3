#define _POSIX_C_SOURCE 200809

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define MAX 80
#define SERVER_TYPE 1

/* keys (same ftok args in client and server) */
#define QUEUE_FTOK_ID 98
#define SHM_FTOK_ID 99 /* shared memory for server count */
#define SEM_FTOK_ID 100 /* semaphore protecting server count */

struct message {
    long mtype;
    char mtext[MAX];
};

/* Globals so signal handler can access them */
int qid = -1;
int semid = -1;
int shmid = -1;
int * server_count = NULL;

/* track whether we created the semaphore (so we can initialize shared state) */
int sem_was_created = 0;

/* union needed for semctl */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

/* do a semaphore P (decrement) or V (increment) on semid, semnum=0
   use SEM_UNDO so kernel will adjust if process dies */
static int sem_op(int semid_local, short op) {
    struct sembuf s;
    s.sem_num = 0;
    s.sem_op = op;
    s.sem_flg = SEM_UNDO;
    return semop(semid_local, &s, 1);
}

/* Parse "PID~text" from inbuf.
   On return: *pid_out receives parsed pid (positive),
   inbuf is adjusted to contain only the text (shifted left).
   Returns 0 on success, -1 on error. */
static int parse_pid_and_shift(char * inbuf, int * pid_out) {
    if (!inbuf || !pid_out) return -1;
    char * tilde = strchr(inbuf, '~');
    if (!tilde) return -1;
    /* extract pid substring */
    size_t pidlen = tilde - inbuf;
    if (pidlen == 0 || pidlen >= 16) return -1; /* sanity bounds */
    char pidbuf[16];
    memcpy(pidbuf, inbuf, pidlen);
    pidbuf[pidlen] = '\0';
    int pid = atoi(pidbuf);
    if (pid <= 0) return -1;
    /* shift text part to beginning */
    char * textstart = tilde + 1;
    memmove(inbuf, textstart, strlen(textstart) + 1);
    *pid_out = pid;
    return 0;
}

/* Cleanup handler: decrement server count; if zero, remove IPC objects.
   IMPORTANT: we only remove the message queue if the shared counting mechanism
   (semaphore + shared memory) was available — otherwise we must NOT delete the queue,
   because other servers may be running but were unable to use the shared mechanism. */
static void cleanup_and_exit(int signo) {
    int local_qid = qid;
    int local_semid = semid;
    int local_shmid = shmid;

    /* If we have a functional semaphore AND valid shared counter, use it to
       decrement and decide whether we're the last server. Only then remove IPC. */
    if (local_semid != -1 && server_count != NULL) {
        if (sem_op(local_semid, -1) == -1) {
            /* Failed to lock; avoid removing IPC as we cannot safely determine count */
            perror("sem_op lock (cleanup)");
            /* Best effort: detach shared memory if attached, but do not remove queue */
            if (server_count) shmdt(server_count);
            exit(EXIT_FAILURE);
        } else {
            /* decrement count */
            (*server_count)--;
            int cnt = *server_count;
            if (sem_op(local_semid, 1) == -1) {
                perror("sem_op unlock (cleanup)");
            }

            if (cnt <= 0) {
                /* last server: remove queue + shm + sem */
                if (local_qid != -1) {
                    if (msgctl(local_qid, IPC_RMID, NULL) == -1) {
                        perror("msgctl IPC_RMID");
                    } else {
                        fprintf(stderr, "Server: removed message queue (last server).\n");
                    }
                }
                /* detach and remove shared memory */
                if (server_count) {
                    shmdt(server_count);
                    server_count = NULL;
                }
                if (local_shmid != -1) {
                    if (shmctl(local_shmid, IPC_RMID, NULL) == -1) {
                        perror("shmctl IPC_RMID");
                    } else {
                        fprintf(stderr, "Server: removed shared memory.\n");
                    }
                }
                /* remove semaphore */
                if (local_semid != -1) {
                    if (semctl(local_semid, 0, IPC_RMID) == -1) {
                        perror("semctl IPC_RMID");
                    } else {
                        fprintf(stderr, "Server: removed semaphore.\n");
                    }
                }
                exit(EXIT_SUCCESS);
            } else {
                /* not the last server: detach and exit, do NOT remove queue */
                if (server_count) {
                    shmdt(server_count);
                    server_count = NULL;
                }
                fprintf(stderr, "Server: other servers remain (count=%d). Exiting without removing IPC.\n", cnt);
                exit(EXIT_SUCCESS);
            }
        }
    } else {
        /* Shared-count mechanism not available: do NOT remove the message queue.
           We cannot know whether other servers are using it, so be conservative.
           Just detach shared memory (if attached) and exit. */
        if (server_count) {
            shmdt(server_count);
            server_count = NULL;
        }
        fprintf(stderr, "Server: shared-count not available; exiting without removing message queue.\n");
        exit(EXIT_SUCCESS);
    }
}

int main(void) {
    key_t keyq, keyshm, keysem;
    struct message kom;
    ssize_t rcvlen;

    /* Create/get the message queue */
    keyq = ftok(".", QUEUE_FTOK_ID);
    if (keyq == (key_t) - 1) {
        perror("ftok(queue)");
        exit(EXIT_FAILURE);
    }

    qid = msgget(keyq, IPC_CREAT | 0666);
    if (qid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    /* Create/get semaphore and shared memory used to count active servers */
    keysem = ftok(".", SEM_FTOK_ID);
    keyshm = ftok(".", SHM_FTOK_ID);
    if (keysem == (key_t) - 1 || keyshm == (key_t) - 1) {
        perror("ftok (shm/sem)");
        /* still proceed with queue-only mode */
    }

    /* Try to create semaphore (exclusive); if exists, just get it */
    sem_was_created = 0;
    semid = -1;
    if (keysem != (key_t)-1) {
        semid = semget(keysem, 1, IPC_CREAT | IPC_EXCL | 0666);
        if (semid != -1) {
            /* newly created semaphore: initialize to 1 */
            union semun arg;
            arg.val = 1;
            if (semctl(semid, 0, SETVAL, arg) == -1) {
                perror("semctl SETVAL");
            } else {
                sem_was_created = 1;
            }
        } else {
            /* semaphore exists or failed to create exclusive; try to get it */
            semid = semget(keysem, 1, IPC_CREAT | 0666);
            if (semid == -1) {
                /* no semaphore available; set semid = -1 and proceed */
                semid = -1;
            }
        }
    }

    /* Create/get shared memory (server_count) */
    shmid = -1;
    server_count = NULL;
    if (keyshm != (key_t)-1) {
        shmid = shmget(keyshm, sizeof(int), IPC_CREAT | 0666);
        if (shmid == -1) {
            perror("shmget");
            shmid = -1;
        } else {
            server_count = (int *) shmat(shmid, NULL, 0);
            if (server_count == (void *) -1) {
                perror("shmat");
                server_count = NULL;
            } else {
                /* If we created the semaphore, we should ensure server_count is initialized */
                if (sem_was_created && server_count != NULL) {
                    /* safe initialize */
                    *server_count = 0;
                }
            }
        }
    }

    if (semid != -1 && server_count != NULL) {
        if (sem_op(semid, -1) == -1) {
            perror("sem_op lock (startup)");
        } else {
            /* Sanity check & increment */
            if (*server_count < 0 || *server_count > 1000000) *server_count = 0;
            (*server_count)++;
            fprintf(stderr, "Server: running servers count = %d\n", *server_count);
            if (sem_op(semid, 1) == -1) perror("sem_op unlock (startup)");
        }
    } else {
        /* no server-count mechanism available */
        fprintf(stderr, "Server: running without shared server-count (single-server semantics).\n");
    }

    /* Install SIGINT handler to call cleanup_and_exit */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup_and_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    printf("Server: ready. Listening for client messages (type %d)...\n", SERVER_TYPE);

    for (;;) {
        memset(&kom, 0, sizeof(kom));
        rcvlen = msgrcv(qid, &kom, sizeof(kom.mtext), (long) SERVER_TYPE, 0);
        if (rcvlen == -1) {
            if (errno == EINTR) continue; /* interrupted by signal */
            perror("msgrcv");
            continue;
        }

        kom.mtext[sizeof(kom.mtext) - 1] = '\0'; /* ensure termination */

        /* parse PID and move text to front */
        int client_pid = 0;
        if (parse_pid_and_shift(kom.mtext, &client_pid) == -1) {
            fprintf(stderr, "Server: malformed message (no PID~prefix): '%s'\n", kom.mtext);
            continue;
        }

        /* convert remaining text in kom.mtext to uppercase */
        for (size_t i = 0; kom.mtext[i] != '\0' && i < sizeof(kom.mtext); ++i)
            kom.mtext[i] = (char) toupper((unsigned char) kom.mtext[i]);

        /* prepare reply: set mtype to client PID and mtext to uppercase text */
        kom.mtype = (long) client_pid;

        if (msgsnd(qid, &kom, strlen(kom.mtext) + 1, 0) == -1) {
            perror("msgsnd (reply)");
            continue;
        }

        printf("Server: processed request from pid=%d -> replied.\n", client_pid);
    }

    /* unreachable, but keep cleanup path */
    cleanup_and_exit(0);
    return EXIT_SUCCESS;
}
