#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char buffer[1024];
    size_t bytes_read;
    FILE *pipe;

    pipe = popen("cat p*.c", "r");
    if (pipe == NULL) {
        perror("popen");
        return EXIT_FAILURE;
    }

    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, pipe);
    if (bytes_read == 0 && ferror(pipe)) {
        perror("fread");
        pclose(pipe);
        return EXIT_FAILURE;
    }

    buffer[bytes_read] = '\0';

    printf("Read data:\n%s\n", buffer);

    if (pclose(pipe) == -1) {
        perror("pclose");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

