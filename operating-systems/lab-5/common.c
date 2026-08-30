#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
#if defined(__linux__)
    struct seminfo *__buf;
#endif
};

static int g_shmid = -1;
static void *g_shmaddr = NULL;
static bool g_shm_master = false;

static void ensure(bool cond, const char *err) {
    if (!cond) {
        fprintf(stderr, "ERROR: %s (errno=%d: %s)\n", err, errno, strerror(errno));
        abort();
    }
}

static void cleanup_shm(void) {
    if (g_shmaddr != NULL) {
        shmdt(g_shmaddr);
        g_shmaddr = NULL;
    }
    if (g_shmid != -1 && g_shm_master) {
        shmctl(g_shmid, IPC_RMID, NULL);
        g_shmid = -1;
    }
}

void *createSHM(ull size, bool master) {
    key_t key = ftok(".", 'z');
    ensure(key != (key_t)-1, "ftok for shm failed");

    g_shmid = shmget(key, (size_t)size, master ? CREVAL : 0666);
    ensure(g_shmid >= 0, "shmget failed");

    g_shmaddr = shmat(g_shmid, NULL, 0);
    ensure(g_shmaddr != (void *)-1, "shmat failed");

    g_shm_master = master;
    if (master) {
        if (atexit(cleanup_shm) != 0) {
            perror("atexit");
        }
    } else {
        if (atexit(cleanup_shm) != 0) {
            perror("atexit");
        }
    }

    return g_shmaddr;
}

static int g_semid = -1;

static void cleanup_sem(void) {
    if (g_semid != -1 && g_shm_master) {
        semctl(g_semid, 0, IPC_RMID);
        g_semid = -1;
    }
}

void createSemaphores(bool master) {
    key_t key = ftok(".", 'a');
    ensure(key != (key_t)-1, "ftok for sem failed");

    if (master) {
        g_semid = semget(key, SEM_COUNT, CREVAL);
    } else {
        g_semid = semget(key, SEM_COUNT, 0666);
    }
    ensure(g_semid >= 0, "semget failed");

    g_shm_master = g_shm_master || master;
    if (master) {
        if (atexit(cleanup_sem) != 0) perror("atexit(sem)");
    }
}

void sem(unsigned short n, int val) {
    struct sembuf op;
    op.sem_num = n;
    op.sem_op = val;
    op.sem_flg = 0;
    if (semop(g_semid, &op, 1) == -1) {
        perror("semop");
    }
}

void semset(unsigned short n, int val) {
    union semun arg;
    arg.val = val;
    if (semctl(g_semid, n, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        abort();
    }
}

int randbound(int bound) {
    if (bound <= 0) return 0;
    return rand() % bound;
}

void randwait(void) {
    int ms = randbound(2000); /* 0..1999 ms */
    printf("\t[Process %d]: waiting %d ms\n", (int)getpid(), ms);
    fflush(stdout);
    usleep((useconds_t)ms * 1000U);
}

