#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "task_queue.h"

typedef struct thread_pool {
    task_queue task_queue;

    pthread_t *threads;

    volatile int running;
    volatile int n_threads;
    volatile int active;

    void (*inactive_callback)(void);
} thread_pool;

thread_pool *thread_pool_create(int n_workers, void (*inactive_callback)(void));
int thread_pool_add(thread_pool *threadpool, void (*handler)(void*), void (*destructor)(void*), void *args);
void try_revive(thread_pool *pool);
void thread_pool_destroy(thread_pool *pool);
#endif
