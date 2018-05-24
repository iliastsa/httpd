#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>

#include "thread_pool.h"

typedef struct {
    pthread_mutex_t lock;

    unsigned long long page_count;
    unsigned long long byte_count;
} ServerStats;

typedef struct {
    // HTTP request and command ports
    int serving_port;
    int command_port;

    // Root directory
    char *root_dir;

    // Server startup time
    struct timeval t_start;

    // Thread pool
    thread_pool *thread_pool;

    // HTTP socket fd
    int http_socket;

    // Command socket fd
    int cmd_socket;

    // HTTP socket structs
    struct sockaddr_in http_in;

    // Command socket structs
    struct sockaddr_in cmd_in;

    // Server statistics
    ServerStats stats;
} ServerResources;

typedef struct {
    int fd;
    char *root_dir;
    ServerStats *stats;
} AcceptArgs;

#endif
