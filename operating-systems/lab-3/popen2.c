#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    FILE *pipe;

    pipe = popen("sort", "w");
    if (pipe == NULL) {
        perror("popen");
        return EXIT_FAILURE;
    }

    fprintf(pipe, "Ala\n");
    fprintf(pipe, "Ania\n");
    fprintf(pipe, "As\n");
    fprintf(pipe, "Kasia\n");
    fprintf(pipe, "Ula\n");
    fprintf(pipe, "Asia\n");
    fprintf(pipe, "Miko\n");

    if (pclose(pipe) == -1) {
        perror("pclose");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

