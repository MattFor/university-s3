#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>

#include "operations.h"

void c1(void) {
  printf("C: doing c1, pid %d\n", (int) getpid());
  sleep(3);
}

void c2(void) {
  printf("C: doing c2, pid %d\n", (int) getpid());
  sleep(3);
}

void c3(void) {
  printf("C: doing c3, pid %d\n", (int) getpid());
  sleep(3);
}

int main(void) {
  key_t key;
  int semID;
  const int N = 4;

  if ((key = ftok(".", 'A')) == -1) {
    perror("ftok (C)");
    exit(EXIT_FAILURE);
  }

  semID = alokujSemafor(key, N, IPC_CREAT | 0666);
  if (semID == -1) {
    perror("alokujSemafor (C)");
    exit(EXIT_FAILURE);
  }

  c1();
  c2();
  signalSemafor(semID, 1);
  c3();
  signalSemafor(semID, 2);

  return EXIT_SUCCESS;
}
