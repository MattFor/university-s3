#include <stdio.h>

#include "../common.h"

int main(void) {
    struct SHMObj *ref = createSHM(sizeof(struct SHMObj), false);
    createSemaphores(false);
    srand((unsigned int)(time(NULL) ^ getpid()));

    for (int i = 0; i < WRITER_ATTEMPTS; ++i) {
        printf("[Writer %d]: I want to write\n", (int)getpid());
        sem(SEM_WR_LOCK, -READER_COUNT);
        int idx = randbound(MEMSIZE);
        int newval = randbound(1000);
        printf("[Writer %d]: Writing memory cell %d. Used to be %d. Now %d.\n",
               (int)getpid(), idx, ref->memory[idx], newval);
        ref->memory[idx] = newval;
        sem(SEM_WR_LOCK, READER_COUNT);
        randwait();
    }
    return 0;
}

