#define _POSIX_C_SOURCE 200809L

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <fcntl.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <time.h>

#include <errno.h>

#define FIFO_NAME "pFIFO"

int main(void) {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // child

        if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);

        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("fork (grandchild)");
            exit(EXIT_FAILURE);
        }

        if (pid2 == 0) {
            unsigned long long seconds;
            int fd;

            printf("Enter number of seconds since the Epoch: ");
            fflush(stdout);

            if (scanf("%llu", & seconds) != 1) {
                fprintf(stderr, "Invalid input\n");
                exit(EXIT_FAILURE);
            }

            fd = open(FIFO_NAME, O_WRONLY);
            if (fd == -1) {
                perror("open FIFO (write)");
                exit(EXIT_FAILURE);
            }

            if (write(fd, & seconds, sizeof(seconds)) != sizeof(seconds)) {
                perror("write FIFO");
                close(fd);
                exit(EXIT_FAILURE);
            }

            close(fd);
            exit(EXIT_SUCCESS);

        } else {
            unsigned long long seconds;
            int fd = open(FIFO_NAME, O_RDONLY);
            if (fd == -1) {
                perror("open FIFO (read)");
                exit(EXIT_FAILURE);
            }

            if (read(fd, & seconds, sizeof(seconds)) != sizeof(seconds)) {
                perror("read FIFO");
                close(fd);
                exit(EXIT_FAILURE);
            }

            close(fd);
            unlink(FIFO_NAME);

            time_t t = (time_t) seconds;
            struct tm * info = gmtime( & t);
            if (info == NULL) {
                perror("gmtime");
                exit(EXIT_FAILURE);
            }

            printf("UTC time: %02d:%02d:%02d, %02d/%02d/%04d\n",
                info -> tm_hour,
                info -> tm_min,
                info -> tm_sec,
                info -> tm_mday,
                info -> tm_mon + 1,
                info -> tm_year + 1900);

            int year = info -> tm_year;
            if (write(pipefd[1], & year, sizeof(year)) != sizeof(year)) {
                perror("write pipe");
                close(pipefd[1]);
                exit(EXIT_FAILURE);
            }

            close(pipefd[1]);
            exit(EXIT_SUCCESS);
        }

    } else {
        // parent

        close(pipefd[1]);

        int year;
        if (read(pipefd[0], & year, sizeof(year)) != sizeof(year)) {
            perror("read pipe");
            close(pipefd[0]);
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);

        printf("Updated year: %d\n", year + 1900);
        exit(EXIT_SUCCESS);
    }
}
