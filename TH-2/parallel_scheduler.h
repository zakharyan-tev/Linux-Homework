#ifndef PARALLEL_SCHEDULER_H
#define PARALLEL_SCHEDULER_H

#include <pthread.h>
#include <functional>
#include <queue>

struct work_item {
    std::function<void()> job;
};

class parallel_scheduler {
public:
    explicit parallel_scheduler(int max_threads);
    ~parallel_scheduler();

    void run(void (*fn)(int), int value);

private:
    int thread_count;
    int terminate_flag = 0;

    std::queue<work_item*> task_queue;
    pthread_t* worker_threads;

    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond_task = PTHREAD_COND_INITIALIZER;

    static void* thread_entry(void* context);
};

#endif
