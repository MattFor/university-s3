#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define PROCESS_COUNT 12
#define BUFFER_SIZE   10
#define SHM_SIZE      12

#define MSG_EMPTY 1
#define MSG_FULL  2

struct buffer {
    long mtype;
    int  mvalue;
};

int shmID = -1;
int semID = -1;
int msgID = -1;

void cleanup(int sig)
{
    if (msgID != -1) msgctl(msgID, IPC_RMID, NULL);
    if (shmID != -1) shmctl(shmID, IPC_RMID, NULL);
    if (semID != -1) semctl(semID, 0, IPC_RMID);

    printf("\nMAIN: cleanup (signal %d)\n", sig);
    exit(EXIT_SUCCESS);
}

int main(void)
{
    key_t k_msg, k_shm, k_sem;
    struct buffer msg;
    struct sigaction sa;
    int i;

    sa.sa_handler = cleanup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    k_msg = ftok(".", 'A');
    k_shm = ftok(".", 'B');
    k_sem = ftok(".", 'C');

    msgID = msgget(k_msg, IPC_CREAT | IPC_EXCL | 0666);
    shmID = shmget(k_shm, SHM_SIZE * sizeof(int), IPC_CREAT | IPC_EXCL | 0666);

    semID = semget(k_sem, 3, IPC_CREAT | IPC_EXCL | 0666);

    semctl(semID, 0, SETVAL, 1); /* SEM_IDX */
    semctl(semID, 1, SETVAL, 1); /* SEM_BUF */
    semctl(semID, 2, SETVAL, 1); /* SEM_IO */

    int *shm_ptr = shmat(shmID, NULL, 0);
    shm_ptr[BUFFER_SIZE]     = 0; /* READ_INDEX */
    shm_ptr[BUFFER_SIZE + 1] = 0; /* WRITE_INDEX */
    shmdt(shm_ptr);

    msg.mtype = MSG_EMPTY;
    msg.mvalue = 0;

    for (i = 0; i < BUFFER_SIZE; i++)
        msgsnd(msgID, &msg, sizeof(int), 0);

    for (i = 0; i < PROCESS_COUNT; i++)
        if (fork() == 0)
            execl("./producent", "producent", NULL);

    for (i = 0; i < PROCESS_COUNT; i++)
        if (fork() == 0)
            execl("./konsumer", "konsumer", NULL);

    for (i = 0; i < 2 * PROCESS_COUNT; i++)
        wait(NULL);

    cleanup(0);
}
