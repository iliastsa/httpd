#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include "task.h"
#include "pthread.h"

typedef struct task_q_node {
    task task;
    struct task_q_node *next;
} task_q_node;

typedef struct task_q {
    task_q_node *head;
    task_q_node *tail;

    int n_tasks;

    // Task queue synchronization.
    pthread_mutex_t queue_rwlock;
    pthread_cond_t  queue_available;
} task_queue;

int task_queue_init(task_queue *task_queue);

task_q_node *task_queue_get(task_queue *task_queue);
int task_queue_put(task_queue *task_queue, task *task);

void task_queue_free(task_queue *queue);
#endif
