/* producent.c — użycie 3 semaforów (SEM_IDX, SEM_BUF, SEM_IO) */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define BUFFER_SIZE 10
#define SHM_SIZE    12

#define MSG_EMPTY 1
#define MSG_FULL  2

struct buffer {
    long mtype;
    int  mvalue;
};

int *shm;

#define READ_INDEX  shm[BUFFER_SIZE]
#define WRITE_INDEX shm[BUFFER_SIZE + 1]

/* semaphores indices */
#define SEM_IDX 0
#define SEM_BUF 1
#define SEM_IO  2

int main(void)
{
    key_t k_msg = ftok(".", 'A');
    key_t k_shm = ftok(".", 'B');
    key_t k_sem = ftok(".", 'C');

    int msgID = msgget(k_msg, 0);
    int shmID = shmget(k_shm, SHM_SIZE * sizeof(int), 0);
    int semID = semget(k_sem, 3, 0);

    shm = shmat(shmID, NULL, 0);

    struct buffer msg;

    printf("PRODUCER started (%d)\n", getpid());

    msgrcv(msgID, &msg, sizeof(int), MSG_EMPTY, 0);

    struct sembuf ops_lock[2] = {
        { SEM_IDX, -1, 0 },
        { SEM_BUF, -1, 0 }
    };
    semop(semID, ops_lock, 2);

    shm[WRITE_INDEX] = getpid();

    /* protected IO */
    struct sembuf io_lock = { SEM_IO, -1, 0 };
    semop(semID, &io_lock, 1);
    printf("PRODUCER %d → index %d\n", getpid(), WRITE_INDEX);
    io_lock.sem_op = 1;
    semop(semID, &io_lock, 1);

    WRITE_INDEX = (WRITE_INDEX + 1) % BUFFER_SIZE;

    struct sembuf ops_unlock[2] = {
        { SEM_BUF, 1, 0 },
        { SEM_IDX, 1, 0 }
    };
    semop(semID, ops_unlock, 2);

    msg.mtype = MSG_FULL;
    msgsnd(msgID, &msg, sizeof(int), 0);

    return 0;
}
