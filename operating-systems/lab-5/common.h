#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdbool.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

typedef unsigned long long ull;

#if defined(VARIANT_READER_PRIORITY)
  #define SEM_RD_LOCK 0
  #define SEM_COUNT   1
#elif defined(VARIANT_WRITER_PRIORITY)
  #define SEM_WR_LOCK 0
  #define SEM_RD_LOCK 1
  #define SEM_COUNT   2
#else
  #error "No variant set! Compile with -DVARIANT_READER_PRIORITY or -DVARIANT_WRITER_PRIORITY"
#endif

#define CREVAL (IPC_CREAT | 0777)

void *createSHM(ull size, bool master);

void createSemaphores(bool master);
void semset(unsigned short n, int val);
void sem(unsigned short n, int val);

/* Misc helpers */
int randbound(int bound);
void randwait(void);

#define MEMSIZE 15
struct SHMObj {
    int memory[MEMSIZE];
};

#define READER_COUNT   10
#define READER_ATTEMPTS 4
#define WRITER_ATTEMPTS 4

