#define _GNU_SOURCE
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>

#include "command_manager.h"
#include "request_manager.h"
#include "server_types.h"
#include "utils.h"

#define HTTP 0
#define CMD  1

volatile static sig_atomic_t check_workers = 0; 

static
void block_thread_signals(sigset_t *oldset) {
    sigset_t new_set;

    sigemptyset(&new_set);

    sigaddset(&new_set, SIGINT);
    sigaddset(&new_set, SIGTERM);
    sigaddset(&new_set, SIGALRM);

    pthread_sigmask(SIG_SETMASK, &new_set, oldset);
}

static
void unblock_thread_signals(sigset_t *set) {
    pthread_sigmask(SIG_SETMASK, set, NULL);
}

static
void alarm_handler(int signum) {
    check_workers = 1;
}

static 
void setup_server_signals() {
    sigset_t set;

    sigemptyset(&set);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, alarm_handler);
}

/*
 * Create a new server and initialize it.
 *
 * params:
 * - char *s_port  : The serving port.
 * - char *c_port  : The command port.
 * - int n_threads : The number of threads we want to have.
 * - char *r_dir   : The root directory.
 *
 * Returns:
 * - A new server if no error occurred.
 * - NULL otherwise.
 */
ServerResources *server_create(int s_port, int c_port, int n_threads, char *r_dir) {
    ServerResources *server = (ServerResources*) malloc(sizeof(ServerResources));

    if (server == NULL) {
        ERR("Memory allocation during server creation failed");
        return NULL;
    }

    // Initialize fds to -1, so we know they are unset
    server->http_socket = -1;
    server->cmd_socket  = -1;

    // Set ports
    server->serving_port = s_port; 
    server->command_port = c_port; 

    // Set root_dir
    server->root_dir = realpath(r_dir, NULL);

    if (server->root_dir == NULL) {
        P_ERR("Failed to expand root directory path", errno);
        free(server);
        return NULL;
    }

    // Check if directory exists and is readable
    if (check_dir_access(server->root_dir) < 0) {
        P_ERR("Could not access provided root directory", errno);
        free(server->root_dir);
        free(server);
        return NULL;
    }

    // Read current time
    if (gettimeofday(&server->t_start, NULL) < 0) {
        P_ERR("Failed to get startup time", errno);
        free(server);
        return NULL;
    }

    // Set stats
    server->stats.page_count = 0;
    server->stats.byte_count = 0;

    int err;
    if ((err = pthread_mutex_init(&server->stats.lock, NULL))) {
        P_ERR("Failed to initialize server stats mutex", err);
        free(server);
        return NULL;
    }

    sigset_t sig_set;

    setup_server_signals();
    block_thread_signals(&sig_set);

    // Create thread pool
    server->thread_pool = thread_pool_create(n_threads, NULL);

    unblock_thread_signals(&sig_set);

    if (server->thread_pool == NULL) {
        ERR("Thread pool creation failed");
        pthread_mutex_destroy(&server->stats.lock);
        free(server);
        return NULL;
    }

    fprintf(stderr, "Server parameters and thread pool intialized\n");
    fprintf(stderr, "Root Directory : %s\n", server->root_dir);

    fprintf(stderr, "HTTP port : %d   CMD port : %d\n", server->serving_port, server->command_port);

    return server;
}

/*
 * Destructor for server.
 *
 * Params:
 * - ServerResources *server : The server we want to free.
 *
 * Returns: -
 */
void free_server(ServerResources *server) {
    if (server == NULL)
        return;

    if (server->http_socket != -1)
        close(server->http_socket);

    if (server->cmd_socket != -1)
        close(server->cmd_socket);

    if (server->root_dir != NULL)
        free(server->root_dir);

    // Destroy thread pool
    thread_pool_destroy(server->thread_pool);

    // Free stats mutex
    pthread_mutex_destroy(&server->stats.lock);

    P_DEBUG("Server stats: Bytes : %lld Pages : %lld\n", server->stats.byte_count, server->stats.page_count);

    free(server);
}

/*
 * Synchronized update for the server statistics.
 *
 * Params:
 * - ServerStats *stats         : The struct containing all the server stats.
 * - unsigned long long n_bytes : The number of bytes we want to increment the byte
 *                                counter by.
 *
 * Returns: -
 */
void update_stats(ServerStats *stats, unsigned long long bytes) {
    int err;
    if((err = pthread_mutex_lock(&stats->lock))) {
        P_ERR("Could not acquire lock for stats", err);
        return;
    }

    stats->byte_count += bytes;
    stats->page_count++;

    if((err = pthread_mutex_unlock(&stats->lock)))
        P_ERR("Could not unlock lock for stats", err);
}

/*
 * Synchronized getter for the server stats. Synchronization is needed to avoid
 * atomicity violations.
 *
 * Params:
 * - ServerStats *src  : The struct we want to copy from.
 * - ServerStats *dest : The struct we want to copy to.
 *
 * Returns:
 * -  0 if no error occurred.
 * - -1 otherwise.
 */
int get_stats_instance(ServerStats *src, ServerStats *dest) {
    int err;
    if((err = pthread_mutex_lock(&src->lock))) {
        P_ERR("Could not acquire lock for stats", err);
        return -1;
    }

    dest->byte_count = src->byte_count;
    dest->page_count = src->page_count;

    if((err = pthread_mutex_unlock(&src->lock))) {
        P_ERR("Could not unlock lock for stats", err);
        return -1;
    }

    return 0;
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

/*
 * Initialize all server sockets.
 *
 * Params:
 * - ServerResources *server : The struct containing all the server resources.
 *
 * Returns:
 * -  0 if no error occurred.
 * - -1 otherwise.
 */
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

/*
 * The main event loop.
 *
 * Params:
 * - ServerResources *server : The struct containing all the server resources.
 *
 * Returns:
 * - 0, always.
 */
char server_run(ServerResources *server) {
    struct pollfd sockets[2];

    const int chk_worker_period = 5;

    sockets[HTTP].fd     = server->http_socket;
    sockets[HTTP].events = POLLIN;

    sockets[CMD].fd     = server->cmd_socket;
    sockets[CMD].events = POLLIN;

    alarm(chk_worker_period);

    for(;;) {
        int status = poll(sockets, 2, -1);

        // Check if timer expired, poll is the longest waiting call in this loop
        if (check_workers) {
            try_revive(server->thread_pool);
            check_workers = 0;

            alarm(chk_worker_period);
        }

        if (status == 0)
            break;
        if (status < 0)
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    P_DEBUG("Something went wrong with poll\n");
                    continue;
            }

        if (sockets[HTTP].revents & POLLIN) {
            int fd;
            if ((fd = accept(sockets[HTTP].fd, NULL, NULL)) < 0) {
                P_ERR("Error accepting connection", errno);
            }
            else {
                P_DEBUG("Incoming fd : %d\n", fd);
                // Allocate int to pass to thread
                AcceptArgs *params = (AcceptArgs*) malloc(sizeof(AcceptArgs));
                if (params == NULL) {
                    P_ERR("Error allocation memory for thread arg", errno);
                    close(fd);
                }
                else {
                    // Prepare parameters to be passed to handler function
                    params->fd       = fd;
                    params->root_dir = server->root_dir;
                    params->stats    = &server->stats;

                    if (thread_pool_add(server->thread_pool, accept_http, NULL, params) < 0) {
                        free(params);
                        ERR("Failed to insert task to thread pool queue");
                    }
                }
            }
        }

        if (sockets[CMD].revents & POLLIN) {
            int fd;
            if ((fd = accept(sockets[CMD].fd, NULL, NULL)) < 0) {
                P_ERR("Error accepting connection", errno);
            }
            else {
                P_DEBUG("Accept command connection : %d\n", fd);

                if (accept_command(fd, server) == CMD_SHUTDOWN)
                    break;
            }
        }

        // Check if timer expired
        if (check_workers) {
            try_revive(server->thread_pool);
            check_workers = 0;

            alarm(chk_worker_period);
        }

    }

    return 0;
}
