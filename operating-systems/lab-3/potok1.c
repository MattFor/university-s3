#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
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
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2 (child)");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        execlp("who", "who", (char *)NULL);

        perror("execlp who");
        exit(EXIT_FAILURE);

    } else {
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2 (parent)");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        execlp("wc", "wc", "-l", (char *)NULL);

        perror("execlp wc");
        exit(EXIT_FAILURE);
    }
}
