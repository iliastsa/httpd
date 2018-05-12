#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "task_queue.h"

typedef struct thread_pool {
    task_queue task_queue;

    pthread_t *threads;

    volatile int running;
    volatile int n_threads;
} thread_pool;

int thread_pool_add(thread_pool *threadpool, void (*handler)(void*), void *args);
void thread_pool_destroy(thread_pool *pool);
#endif
