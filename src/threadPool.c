
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "threadPool.h"

static void* thread_pool_worker(void *arg) {
    thread_pool *pool = (thread_pool *)arg;

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        while ((pool->queue_size == 0) && !(pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->shutdown && (pool->queue_size == 0)) {
            break;
        }

        thread_pool_task task = pool->queue[pool->head];
        pool->head = (pool->head + 1) % pool->queue_max;
        pool->queue_size--;

        pthread_mutex_unlock(&(pool->lock));
        (*(task.function))(task.argument);
    }

    pool->thread_count--;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return NULL;
}

thread_pool* thread_pool_create(int thread_count, int queue_max) {
  thread_pool *pool = (thread_pool *)malloc(sizeof(thread_pool));
    if (pool == NULL) return NULL;

    pool->thread_count = 0;
    pool->queue_max = queue_max;
    pool->queue_size = 0;
    pool->head = 0;
    pool->tail = 0;
    pool->shutdown = 0;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->queue = (thread_pool_task *)malloc(sizeof(thread_pool_task) * queue_max);

    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, thread_pool_worker, (void*)pool) == 0) {
            pool->thread_count++;
            pthread_detach(pool->threads[i]); 
        }
    }

    printf("[ThreadPool] Đã khởi tạo thành công %d Luồng công nhân.\n", pool->thread_count);
    return pool;
}

int thread_pool_add(thread_pool *pool, void (*function)(void*), void *argument){
    pthread_mutex_lock(&(pool->lock));

    if ((pool->queue_size == pool->queue_max) || pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
        return -1; 
    }

    pool->queue[pool->tail].function = function;
    pool->queue[pool->tail].argument = argument;
    pool->tail = (pool->tail + 1) % pool->queue_max;
    pool->queue_size++;

    pthread_cond_signal(&(pool->notify));

    pthread_mutex_unlock(&(pool->lock));
    return 0; 
}

int thread_pool_destroy(thread_pool *pool) {
    if (pool == NULL) return -1;
    pthread_mutex_lock(&(pool->lock));

    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->notify)); 
    pthread_mutex_unlock(&(pool->lock));

    usleep(10000); 
    free(pool->threads);
    free(pool->queue);
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool);
    printf("[ThreadPool] Đã giải phóng toàn bộ Pool.\n");
    return 0;
}