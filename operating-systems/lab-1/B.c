#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>

#include "operations.h"

void b1(void) {
  printf("B: doing b1, pid %d\n", (int) getpid());
  sleep(2);
}

void b2(void) {
  printf("B: doing b2, pid %d\n", (int) getpid());
  sleep(2);
}

void b3(void) {
  printf("B: doing b3, pid %d\n", (int) getpid());
  sleep(2);
}

int main(void) {
  key_t key;
  int semID;
  const int N = 4;

  if ((key = ftok(".", 'A')) == -1) {
    perror("ftok (B)");
    exit(EXIT_FAILURE);
  }

  semID = alokujSemafor(key, N, IPC_CREAT | 0666);
  if (semID == -1) {
    perror("alokujSemafor (B)");
    exit(EXIT_FAILURE);
  }

  b1();
  signalSemafor(semID, 0);

  if (waitSemafor(semID, 1, 0) == -1) {
    perror("waitSemafor(1) (B)");
    exit(EXIT_FAILURE);
  }
  b2();

  if (waitSemafor(semID, 3, 0) == -1) {
    perror("waitSemafor(3) (B)");
    exit(EXIT_FAILURE);
  }
  b3();

  return EXIT_SUCCESS;
}
