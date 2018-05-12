#ifndef TASK_H
#define TASK_H

typedef struct task {
    void (*handler)(void*);
    void (*destructor)(void*);
    void *args;
} task;

#endif
