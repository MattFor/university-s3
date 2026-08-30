#include <stdio.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/sem.h>

#include <unistd.h>

#include "operations.h"

int alokujSemafor(key_t klucz, int number, int flagi) {
  int semID;
  if ((semID = semget(klucz, number, flagi)) == -1) {
    perror("Error in semget (alokujSemafor)");
    exit(EXIT_FAILURE);
  }
  return semID;
}

int zwolnijSemafor(int semID, int number) {
  return semctl(semID, number, IPC_RMID, NULL);
}

void inicjalizujSemafor(int semID, int number, int val) {
  if (semctl(semID, number, SETVAL, val) == -1) {
    perror("Error in semctl (inicjalizujSemafor)");
    exit(EXIT_FAILURE);
  }
}

int waitSemafor(int semID, int number, int flags) {
  struct sembuf operacje[1];
  operacje[0].sem_num = number;
  operacje[0].sem_op = -1;
  operacje[0].sem_flg = 0 | flags; /* add SEM_UNDO if desired */

  if (semop(semID, operacje, 1) == -1) {
    /* caller may handle the error; return -1 to signal failure */
    return -1;
  }
  return 1;
}

void signalSemafor(int semID, int number) {
  struct sembuf operacje[1];
  operacje[0].sem_num = number;
  operacje[0].sem_op = 1;
  operacje[0].sem_flg = 0; /* add SEM_UNDO if desired */

  if (semop(semID, operacje, 1) == -1)
    perror("Error in semop (signalSemafor)");
}

int valueSemafor(int semID, int number) {
  return semctl(semID, number, GETVAL, NULL);
}
