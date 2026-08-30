#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define FIFO_NAME "pFIFO"

int main(void)
{
    int fd;

    if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    switch (fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);

        case 0: /* child: producer */
            fprintf(stderr, "Child: starting (who)\n");

            close(STDOUT_FILENO);
            fd = open(FIFO_NAME, O_WRONLY);
            if (fd != STDOUT_FILENO) {
                perror("open FIFO for writing");
                exit(EXIT_FAILURE);
            }

            fprintf(stderr, "Child: executing 'who'\n");
            execlp("who", "who", (char *)NULL);

            perror("execlp who");
            exit(EXIT_FAILURE);

        default: /* parent: consumer */
            close(STDIN_FILENO);
            fd = open(FIFO_NAME, O_RDONLY);
            if (fd != STDIN_FILENO) {
                perror("open FIFO for reading");
                exit(EXIT_FAILURE);
            }

            printf("Parent: executing 'wc -l'\n");
            execlp("wc", "wc", "-l", (char *)NULL);

            perror("execlp wc");
            exit(EXIT_FAILURE);
    }
}

