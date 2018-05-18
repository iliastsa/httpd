#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <time.h>

#include "thread_pool.h"

typedef struct {
    // HTTP request and command ports
    int serving_port;
    int command_port;

    // Root directory
    char *root_dir;

    // Server startup time
    time_t t_start;

    // Thread pool
    thread_pool *thread_pool;
} ServerResources;

#endif
