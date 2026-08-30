#include <stdio.h>

#include "../common.h"

int main(void) {
    struct SHMObj *ref = createSHM(sizeof(struct SHMObj), false);
    createSemaphores(false);
    srand((unsigned int)(time(NULL) ^ getpid()));

    for (int i = 0; i < READER_ATTEMPTS; ++i) {
        sem(SEM_RD_LOCK, -1);
        int idx = randbound(MEMSIZE);
        printf("[Reader %d]: Memory cell %d is %d.\n", (int)getpid(), idx, ref->memory[idx]);
        sem(SEM_RD_LOCK, 1);
        randwait();
    }
    
    return 0;
}

