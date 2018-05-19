#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

#include "server_types.h"
#include "request_manager.h"
#include "utils.h"

#define HTTP 0
#define CMD  1

ServerResources *server_create(int s_port, int c_port, int n_threads, char *r_dir) {
    ServerResources *server = (ServerResources*) malloc(sizeof(ServerResources));

    if (server == NULL) {
        ERR("Memory allocation during server creation failed");
        return NULL;
    }

    // Set ports
    server->serving_port = s_port; 
    server->command_port = c_port; 

    // TODO : First check if root dir is accessible and readable
    // Set root_dir
    server->root_dir = r_dir;

    // Read current time
    time(&server->t_start);

    // Create thread pool
    server->thread_pool = thread_pool_create(n_threads);

    if (server->thread_pool == NULL) {
        ERR("Thread pool creation failed");
        return NULL;
    }

    return server;
}

static
char init_socket(int *sock, struct sockaddr_in *sock_in, int port, int backlog) {
    // Try creating the socket
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        P_ERR("Error when creating socket", errno);
        *sock = -1;
        return -1;
    }

    // Set SO_REUSEADDR
    int on = 1;
    setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

    // Set fields in socket struct
    sock_in->sin_addr.s_addr = htonl(INADDR_ANY);
    sock_in->sin_family      = AF_INET;
    sock_in->sin_port        = htons(port);

    if (bind(*sock, (struct sockaddr *)sock_in, sizeof(struct sockaddr_in)) < 0) {
        P_ERR("Error when binding", errno);
        close(*sock);
        *sock = -1;
        return -1;
    }

    if (listen(*sock, backlog) < 0) {
        P_ERR("Error when listening", errno);
        close(*sock);
        *sock = -1;
        return -1;
    }

    return 0;
}

char server_init_sockets(ServerResources *server, int backlog) {
    printf("Initialzing sockets...\n");

    // Initialize HTTP socket
    if (init_socket(&server->http_socket, &server->http_in, server->serving_port, backlog) < 0) {
        ERR("HTTP socket initialization failed");
        return -1;
    }

    // Initialize Command socket
    if (init_socket(&server->cmd_socket, &server->cmd_in, server->command_port, backlog) < 0) {
        ERR("Command socket initialization failed");
        return -1;
    }

    printf("Done.\n");

    return 0;
}

void thread_accept(void *arg) {
    int fd = *(int*)arg;

    P_DEBUG("Got fd %d\n", fd);
    
    close(fd);
}

char server_run(ServerResources *server) {
    struct pollfd sockets[2];

    sockets[HTTP].fd     = server->http_socket;
    sockets[HTTP].events = POLLIN;

    sockets[CMD].fd     = server->cmd_socket;
    sockets[CMD].events = POLLIN;

    for(;;) {
        // TODO : Check status, and act accordingly
        int status = poll(sockets, 2, -1);

        int fd;
        if (sockets[HTTP].revents & POLLIN) {
            if ((fd = accept(sockets[HTTP].fd, NULL, NULL)) < 0) {
                P_ERR("Error accepting connection", errno);
            }

            P_DEBUG("Incoming fd : %d\n", fd);
            // Allocate int to pass to thread
            AcceptArgs *params = (AcceptArgs*) malloc(sizeof(AcceptArgs));
            if (params == NULL) {
                P_ERR("Error allocation memory for thread arg", errno);
            }

            params->fd = fd;
            params->root_dir = server->root_dir;
            thread_pool_add(server->thread_pool, accept_http, NULL, params);
        }
    }
}