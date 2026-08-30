#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define MAX 80
#define SERVER_TYPE 1

struct message
{
    long mtype;
    char mtext[MAX];
};

int main(void)
{
    key_t key;
    int qid;
    struct message send_msg;
    struct message recv_msg;
    pid_t mypid = getpid();

    key = ftok(".", 98);

    if (key == (key_t)-1)
    {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    qid = msgget(key, IPC_CREAT | 0660);

    if (qid == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("Client %d: started. Type text lines to send to server.\n", mypid);

    while (1)
    {
        char line[MAX];

        printf("Client[%d]: Enter text (empty line to quit): ", mypid);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            // EOF or error
            printf("\nClient[%d]: exiting.\n", mypid);
            break;
        }

        size_t len = strlen(line);

        if (len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
            --len;
        }

        if (len == 0)
        {
            // user requested quit via empty line
            printf("Client[%d]: quitting on user request.\n", mypid);
            break;
        }

        // prepare and send: "PID~message"
        send_msg.mtype = SERVER_TYPE;

        if (snprintf(send_msg.mtext,
                     sizeof(send_msg.mtext),
                     "%d~%s",
                     (int)mypid,
                     line) >= (int)sizeof(send_msg.mtext))
        {
            fprintf(stderr, "Client[%d]: input too long; message truncated.\n", mypid);
            send_msg.mtext[sizeof(send_msg.mtext) - 1] = '\0';
        }

        if (msgsnd(qid, &send_msg, strlen(send_msg.mtext) + 1, 0) == -1)
        {
            perror("msgsnd");
            continue;
        }

        // wait for reply of type == mypid
        if (msgrcv(qid, &recv_msg, sizeof(recv_msg.mtext), (long)mypid, 0) == -1)
        {
            perror("msgrcv");
            continue;
        }

        printf("Client[%d]: Received reply: \"%s\"\n", mypid, recv_msg.mtext);
    }

    return EXIT_SUCCESS;
}
