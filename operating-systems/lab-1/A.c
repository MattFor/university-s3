#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>

#include "operations.h"

void a1(void) {
  printf("A: doing a1, pid %d\n", (int) getpid());
  sleep(1);
}

void a2(void) {
  printf("A: doing a2, pid %d\n", (int) getpid());
  sleep(1);
}

void a3(void) {
  printf("A: doing a3, pid %d\n", (int) getpid());
  sleep(1);
}

int main(void) {
  key_t key;
  int semID;
  const int N = 4;

  if ((key = ftok(".", 'A')) == -1) {
    perror("ftok (A)");
    exit(EXIT_FAILURE);
  }

  semID = alokujSemafor(key, N, IPC_CREAT | 0666);
  if (semID == -1) {
    perror("alokujSemafor (A)");
    exit(EXIT_FAILURE);
  }

  if (waitSemafor(semID, 0, 0) == -1) {
    perror("waitSemafor(0) (A)");
    exit(EXIT_FAILURE);
  }
  a1();

  if (waitSemafor(semID, 2, 0) == -1) {
    perror("waitSemafor(2) (A)");
    exit(EXIT_FAILURE);
  }
  a2();
  a3();

  signalSemafor(semID, 3);

  return EXIT_SUCCESS;
}
