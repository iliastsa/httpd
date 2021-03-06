#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR(msg) fprintf(stderr, "%s\n", msg)
#define P_ERR(msg,err) fprintf(stderr, "%s : %s\n", msg, strerror(err))

#include "thread_pool.h"

static void* thread_run(void *t_pool);

#ifdef TEST_KILL
static int test_kill = 1;
#endif

/*
 * Allocates memory and intializes a thread_pool.
 *
 * Params:
 * - int n_workers                   : The number of workers threads we want to have.
 * - void (*inactive_callback)(void) : The function that will be called when the thread
 *                                     pool becomes in active. 
 *                                     The thread pool is said to be inactive, when the 
 *                                     thread count and task count are simultaniously zero.
 *
 * Returns:
 * - A pointer to the new thread pool we created, if no error occured.
 * - NULL otherwise.
 */
thread_pool *thread_pool_create(int n_workers, void (*inactive_callback)(void)) {
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
    threadpool->active    = 0;

    threadpool->inactive_callback = inactive_callback;

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

    #ifdef TEST_KILL
    test_kill = 0;
    #endif

    return threadpool;
}

/*
 * Add a new task into the thread pool.
 *
 * Params:
 * - void (*handler)(void*)    : The function that we want the thread pool to run.
 * - void (*destructor)(void*) : The function that the thread pool will call, to free the arguments.
 * - void *args                : The arguments that will be passed to the handler.
 *
 * Returns:
 * -  0 if no error occured.
 * - -1 otherwise.
 */
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

/*
 * Looping function that all worker threads run.
 *
 * Params:
 * - void *args : A pointer to the thread pool that owns the
 *                worker thread.
 *
 * Returns:
 * - This function always returns NULL.
 */
static void* thread_run(void *t_pool) {
    thread_pool *pool = (thread_pool*) t_pool;

    #ifdef TEST_KILL
    if (test_kill)
        return NULL;
    #endif

    for (;;) {
        // Acquire lock since the get function requires it
        pthread_mutex_lock(&pool->task_queue.queue_rwlock);

        while (pool->task_queue.n_tasks == 0 && pool->running == 1)
            pthread_cond_wait(&pool->task_queue.queue_available, &pool->task_queue.queue_rwlock);

        if (pool->running == 0 && pool->task_queue.n_tasks == 0)
            break;

        // Get task from queue
        task_q_node *q_node = task_queue_get(&pool->task_queue);

        pool->active++;

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

        pthread_mutex_lock(&pool->task_queue.queue_rwlock);

        pool->active--;

        // Inactivity condition
        if (pool->active == 0 && pool->task_queue.n_tasks == 0 && pool->inactive_callback != NULL)
            pool->inactive_callback();

        pthread_mutex_unlock(&pool->task_queue.queue_rwlock);
    }

    // If we reached here, we are terminating execution, free the lock
    pthread_mutex_unlock(&pool->task_queue.queue_rwlock);

    // Finish execution
    pthread_exit(NULL);

    return NULL;
}

/*
 * Checks if any thread has died, and revives all dead threads.
 *
 * Params:
 * - thread_pool *pool : The thread pool we are checking.
 *
 * Returns: -
 */
void try_revive(thread_pool *pool) {
    for (int i = 0; i < pool->n_threads; ++i) {
        if (pthread_tryjoin_np(pool->threads[i], NULL) == 0) {
            fprintf(stderr ,"Reviving thread with id %d\n", i);
            pthread_create(pool->threads + i, NULL, thread_run, pool);
        }
    }
}

/*
 * Releases all memory and resources associated with the thread pool.
 *
 * Params:
 * - thread_pool *pool : The thread pool we want to free.
 *
 * Returns: -
 */
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
