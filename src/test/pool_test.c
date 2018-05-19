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
        //read_message(newsock, &buf, "\r\n");
        int ret = read_request(newsock, &buf);

        P_DEBUG("Return code is %d\n", ret);
        printf("%s", buf);

        HttpRequest *req = malloc(sizeof(HttpRequest));
        init_request(req);

        if (parse_request(buf, req) != OK )
            P_DEBUG("Request failed\n");
        else {
            P_DEBUG("Request passed!\n");
            P_DEBUG("Here is the requested_file: (%s) \n", req->requested_file);
        }

        print_str_map(req->key_value_pairs);

        free_request(req);
        free(buf);
    }
}

void test_server(){
    ServerResources *server = server_create(9090, 8080, 3, "/home/iliastsa/root_dir");

    server_init_sockets(server, 10);
    server_run(server);
}


extern const char * const response_messages[];

void enum_test(){
    printf(response_messages[NOT_IMPLEMENTED], "");
    printf("\n");
}

int main(void){
    test_server();
    //enum_test();
    return 0;
}
