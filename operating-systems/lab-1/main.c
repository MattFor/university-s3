#include <stdio.h>

#include <unistd.h>

#include <string.h>

#include <stdlib.h>

#include <fcntl.h>

#include <sys/wait.h>

#include "operations.h"

#define PROCESS_COUNT 3 /* number of child processes */

int main(void) {
  key_t key;
  int semID;
  const int SEM_COUNT = 4; /* number of semaphores */
  const char * processes[PROCESS_COUNT] = {
    "A",
    "B",
    "C"
  };
  char path[16];
  int i;

  if ((key = ftok(".", 'A')) == -1) {
    perror("ftok (main)");
    exit(EXIT_FAILURE);
  }

  /* try to create the semaphore set; fail if it already exists */
  semID = alokujSemafor(key, SEM_COUNT, IPC_CREAT | IPC_EXCL | 0666);
  if (semID == -1) {
    perror("alokujSemafor (main)");
    exit(EXIT_FAILURE);
  }

  /* initialize all semaphores to 0 */
  for (i = 0; i < SEM_COUNT; ++i) {
    inicjalizujSemafor(semID, i, 0);
  }

  printf("Semaphores ready!\n");
  fflush(stdout);

  for (i = 0; i < PROCESS_COUNT; ++i) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork (mainprog)");
      zwolnijSemafor(semID, SEM_COUNT);
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      snprintf(path, sizeof(path), "./%s", processes[i]);
      execl(path, processes[i], (char * ) NULL);
      /* if execl returns, it failed */
      perror("execl");
      _exit(EXIT_FAILURE);
    }
    /* parent continues */
  }

  /* wait for all children */
  for (i = 0; i < PROCESS_COUNT; ++i) {
    wait(NULL);
  }

  /* remove semaphore set */
  if (zwolnijSemafor(semID, SEM_COUNT) == -1)
    perror("zwolnijSemafor (main)");

  printf("MAIN: Done.\n");
  return EXIT_SUCCESS;
}
