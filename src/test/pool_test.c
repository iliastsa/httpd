#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "thread_pool.h"

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

//void read_message(int fd, )

void test_socket() {
    struct sockaddr_in server;
    socklen_t clientlen;

    struct sockaddr *serverptr = (struct sockaddr*)&server;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(8282);

    bind(sock, serverptr, sizeof(server));


}

int main(void){
    return 0;
}
