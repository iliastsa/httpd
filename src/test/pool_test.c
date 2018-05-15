#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "thread_pool.h"

#define perror_exit(msg) { perror(msg); exit(-1); }

void test(void *params) {
    printf("Thread %ld : Test func\n", pthread_self());
    sleep(1);
}

void test_threads() {
    thread_pool *pool = thread_pool_create(2);
    thread_pool_add(pool, test, NULL, NULL);
    thread_pool_add(pool, test, NULL, NULL);
    thread_pool_add(pool, test, NULL, NULL);
    thread_pool_add(pool, test, NULL, NULL);
    thread_pool_add(pool, test, NULL, NULL);
    thread_pool_add(pool, test, NULL, NULL);
    thread_pool_destroy(pool);
}

int read_message(int fd, char **msg, const char *delim) {
    int bufsz = 3;
    static char buf[3];

    char *temp    = NULL;
    char *ret_buf = NULL;
    int len = 0;
    int b_read;
    int pattern_idx = 0;
    int goal_idx = strlen(delim);
    while ((b_read=read(fd, buf, bufsz)) > 0) {
        len += b_read;
        temp = malloc (len + 1);

        printf("Read %d bytes from socket\n", b_read);

        if (ret_buf != NULL)
            memcpy(temp, ret_buf, len - b_read);

        for (int i = 0; i < b_read; ++i) {
            *(temp + len - b_read + i) = buf[i];

            if (buf[i] == delim[pattern_idx]) {
                printf("Found %d\n", pattern_idx);
                pattern_idx++;
            }
            else
                pattern_idx = 0;

            if (pattern_idx == goal_idx)
                break;
        }

        free(ret_buf);
        ret_buf = temp;
        ret_buf[len] = '\0';

        if (ret_buf[len-1] == '\n')
            printf("Has newline at end\n");

        if (b_read < bufsz)
            break;

    }

    *msg = ret_buf;
    return len;
}

void test_socket() {
    struct sockaddr_in server, client;
    socklen_t clientlen;

    int port = 9004;


    int newsock;
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        perror_exit("sock");

    int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct sockaddr *clientptr = (struct sockaddr*)&client;

    struct hostent *rem;

    if(bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");

    if (listen(sock, 5) < 0)
        perror_exit("listen");

    printf("Listening for connections to port %d\n", port);

    while (1) {
        clientlen = sizeof(client);

        if ((newsock = accept(sock, clientptr, &clientlen)) < 0)
            perror_exit("accept");

        if ((rem = gethostbyaddr((char*)&client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL )
            perror_exit("gethostbyaddr");

        printf("Accepted connection from %s\n", rem->h_name);

        char *buf;
        read_message(newsock, &buf, "\r\n");

        printf("%s", buf);

        free(buf);
    }
}

int main(void){
    test_socket();
    return 0;
}
