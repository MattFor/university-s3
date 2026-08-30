#include <stdio.h>

#include "../common.h"

int main(void) {
    struct SHMObj *ref = createSHM(sizeof(struct SHMObj), true);
    createSemaphores(true);

    semset(SEM_RD_LOCK, READER_COUNT);
    semset(SEM_WR_LOCK, READER_COUNT);

    printf("[Master]: Initialize shared memory @ %p.\n", (void*)ref);
    for (int i = 0; i < MEMSIZE; ++i) {
        ref->memory[i] = randbound(1000);
        printf("memory[%d] = %d\n", i, ref->memory[i]);
    }

    for (int i = 0; i < READER_COUNT; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            execl("./reader", "./reader", (char *)NULL);
            perror("execl reader");
            _exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork reader");
        }
    }

    pid_t wp = fork();
    if (wp == 0) {
        execl("./writer", "./writer", (char *)NULL);
        perror("execl writer");
        _exit(EXIT_FAILURE);
    } else if (wp < 0) {
        perror("fork writer");
    }

    for (int i = 0; i < READER_COUNT + 1; ++i) wait(NULL);

    printf("Done.\n");
    return 0;
}

