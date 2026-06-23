#ifndef THREADPOOL_H
#define THREADPOOL_H
#pragma once
#include <pthread.h>

typedef struct {
    void (*function)(void*);
    void *argument;
} thread_pool_task;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    thread_pool_task *queue;
    int thread_count;
    int queue_size;
    int queue_max;
    int head;
    int tail;
    int shutdown;
} thread_pool;

thread_pool* thread_pool_create(int thread_count, int queue_max);
int thread_pool_add(thread_pool *pool, void (*function)(void*), void *argument);
int thread_pool_destroy(thread_pool *pool);
#endif // THREADPOOL_H