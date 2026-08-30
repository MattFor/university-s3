#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(void)
{
    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1) {
        perror("pipe1");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork (cat)");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        if (dup2(pipe1[1], STDOUT_FILENO) == -1) {
            perror("dup2 (cat)");
            exit(EXIT_FAILURE);
        }

        close(pipe1[0]);
        close(pipe1[1]);

        execlp("cat", "cat", "potok1.c", (char *)NULL);
        perror("execlp cat");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe2) == -1) {
        perror("pipe2");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork (grep close)");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2 (grep close stdin)");
            exit(EXIT_FAILURE);
        }

        if (dup2(pipe2[1], STDOUT_FILENO) == -1) {
            perror("dup2 (grep close stdout)");
            exit(EXIT_FAILURE);
        }

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        execlp("grep", "grep", "close", (char *)NULL);
        perror("execlp grep close");
        exit(EXIT_FAILURE);
    }

    if (dup2(pipe2[0], STDIN_FILENO) == -1) {
        perror("dup2 (grep pdesk)");
        exit(EXIT_FAILURE);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    execlp("grep", "grep", "pdesk", (char *)NULL);
    perror("execlp grep pdesk");
    exit(EXIT_FAILURE);
}

