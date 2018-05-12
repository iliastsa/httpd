#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR(msg) fprintf(stderr, "%s\n", msg)
#define P_ERR(msg,err) fprintf(stderr, "%s : %s\n", msg, strerror(err))

#include "thread_pool.h"

static void* thread_run(void *t_pool);

thread_pool *thread_pool_create(int n_workers) {
    int err;

    thread_pool *threadpool = (thread_pool*) malloc(sizeof(thread_pool));

    if (threadpool == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
            return NULL;
    }

    if (task_queue_init(&threadpool->task_queue) < 0) {
        fprintf(stderr, "Task queue init failed\n");
        return NULL;
    }

    threadpool->threads = (pthread_t*) malloc(sizeof(pthread_t) * n_workers);

    if ( threadpool->threads == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        task_queue_free(&threadpool->task_queue);
        free(threadpool);
        return NULL;
    }

    threadpool->n_threads = 0;
    threadpool->running   = 1;

    // Initialize threads
    for (int i = 0; i < n_workers; ++i) {
        // If an error occured, destroy threadpool and return NULL
        if ((err = pthread_create(threadpool->threads + i, NULL, thread_run, threadpool)) != 0){
            P_ERR("Thread creation failed", err);
            thread_pool_destroy(threadpool);
            return NULL;
        }

        threadpool->n_threads++;
    }

    return threadpool;
}

int thread_pool_add(thread_pool *threadpool, void (*handler)(void*), void (*destructor)(void*), void *args){
    // Prepare task struct
    task wrapper;
    wrapper.handler = handler;
    wrapper.args    = args;
    wrapper.destructor = destructor;

    if (task_queue_put(&threadpool->task_queue, &wrapper) < 0) {
        fprintf(stderr, "Failed to add task\n");
        return -1;
    }

    return 0;
}

static void* thread_run(void *t_pool) {
    thread_pool *pool = (thread_pool*) t_pool;

    for (;;) {
        // Acquire lock since the get function requires it
        pthread_mutex_lock(&pool->task_queue.queue_rwlock);

        while (pool->task_queue.n_tasks == 0 && pool->running == 1)
            pthread_cond_wait(&pool->task_queue.queue_available, &pool->task_queue.queue_rwlock);

        if (pool->running == 0 && pool->task_queue.n_tasks == 0)
            break;

        // Get task from queue
        task_q_node *q_node = task_queue_get(&pool->task_queue);

        // Unlock mutex, since we completed our read operation
        pthread_mutex_unlock(&pool->task_queue.queue_rwlock);

        // Unwrap the task struct
        void (*handler)(void*)    = q_node->task.handler;
        void (*destructor)(void*) = q_node->task.destructor;
        void *args                = q_node->task.args;

        // Free the queue node
        free(q_node);

        handler(args);

        if (destructor == NULL)
            free(args);
        else
            destructor(args);
    }

    // If we reached here, we are terminating execution, free the lock
    pthread_mutex_unlock(&pool->task_queue.queue_rwlock);

    // Finish execution
    pthread_exit(NULL);

    return NULL;
}

void thread_pool_destroy(thread_pool *pool) {
    if (pool == NULL)
        return;

    pthread_mutex_lock(&pool->task_queue.queue_rwlock);

    pool->running = 0;

    pthread_cond_broadcast(&pool->task_queue.queue_available);

    pthread_mutex_unlock(&pool->task_queue.queue_rwlock);

    for (int i = 0; i < pool->n_threads; ++i)
        pthread_join(pool->threads[i], NULL);

    task_queue_free(&pool->task_queue);
    free(pool->threads);

    free(pool);
}
