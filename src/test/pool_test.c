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

#include "server_manager.h"
#include "thread_pool.h"
#include "request.h"
#include "utils.h"

#define perror_exit(msg) { perror(msg); exit(-1); }

void thread_func(void *params) {
    printf("Thread %ld : Test func\n", pthread_self());
    sleep(1);
}

void test_threads() {
    thread_pool *pool = thread_pool_create(2);
    thread_pool_add(pool, thread_func, NULL, NULL);
    thread_pool_add(pool, thread_func, NULL, NULL);
    thread_pool_add(pool, thread_func, NULL, NULL);
    thread_pool_add(pool, thread_func, NULL, NULL);
    thread_pool_add(pool, thread_func, NULL, NULL);
    thread_pool_add(pool, thread_func, NULL, NULL);
    thread_pool_destroy(pool);
}

void test_server(){
    ServerResources *server = server_create(9090, 8080, 3, "../../../../root_dir");

    if (server == NULL)
        return;

    if (server_init_sockets(server, 10) < 0)
        return;

    server_run(server);
    free_server(server);
}

int main(void){
    test_server();
    return 0;
}
