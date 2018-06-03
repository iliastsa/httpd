#include <stdlib.h>
#include <stdio.h>

#include "task_queue.h"

/*
 * Initialized the task queue resources.
 *
 * Params:
 * - task_queue *task_queue : The task queue to be initialized.
 *
 * Returns:
 *  0 if no error occured.
 * -1 otherwise.
 */
int task_queue_init(task_queue *task_queue) {
    // Initialize basic fields
    task_queue->n_tasks = 0;
    task_queue->head    = NULL;
    task_queue->tail    = NULL;

    // Initialize locks and condition variables
    int err;
    if ((err = pthread_cond_init(&task_queue->queue_available, NULL))){
        fprintf(stderr, "Failed to intialize condition variable\n");
        return -1;
    }

    if ((err = pthread_mutex_init(&task_queue->queue_rwlock, NULL))){ 
        fprintf(stderr, "Failed to intialize rw mutex\n");
        pthread_cond_destroy(&task_queue->queue_available);
        return -1;
    }

    return 0;
}

/*
 * Extracts a task queue node from the queue.
 *
 * The caller IS RESPONSIBLE for locking/unlocking any
 * necessary locks.
 * 
 * Params:
 * - task_queue *task_queue : The task queue we want to extract from.
 *
 * Returns:
 * - The head of the task queue, if it exists.
 * - NULL if the queue is empty.
 */
task_q_node *task_queue_get(task_queue *task_queue) {
    // Return the head of the queue
    task_q_node *ret = task_queue->head;

    // Update queue
    if (task_queue->n_tasks <= 1) {
        task_queue->head    = NULL;
        task_queue->tail    = NULL;
        task_queue->n_tasks = 0;
    }
    else {
        task_queue->head = task_queue->head->next;
        task_queue->n_tasks--;
    }

    return ret;
}

/*
 * Inserts a new task in the task queue.
 *
 * Params:
 * - task_queue *task_queue : The task queue we want to insert into.
 * - task *task             : The task we want to insert.
 *
 * Returns:
 * -  0 if no error occured.
 * - -1 otherwise.
 */
int task_queue_put(task_queue *task_queue, task *task) {
    // Allocate memory for the new queue node.
    task_q_node *new_node = (task_q_node*) malloc(sizeof(task_q_node));

    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // Setup fields
    new_node->task.handler    = task->handler;
    new_node->task.args       = task->args;
    new_node->task.destructor = task->destructor;
    new_node->next         = NULL;

    // Lock queue
    pthread_mutex_lock(&task_queue->queue_rwlock);

    // Insert into queue
    if (task_queue->n_tasks == 0) {
        task_queue->head = new_node;
        task_queue->tail = new_node;
    }
    else {
        task_queue->tail->next = new_node;
        task_queue->tail       = new_node;
    }

    // Increment counter
    task_queue->n_tasks++;

    // Signal any thread waiting to read from the queue
    pthread_cond_signal(&task_queue->queue_available);

    // Unlock the lock
    pthread_mutex_unlock(&task_queue->queue_rwlock);

    return 0;
}

/*
 * Frees all resources associated with a task queue.
 *
 * Params:
 * - task_queue *task_queue : The task queue we want to free.
 *
 * Returns: -
 */
void task_queue_free(task_queue *queue) {
    pthread_mutex_destroy(&queue->queue_rwlock);
    pthread_cond_destroy(&queue->queue_available);
}
