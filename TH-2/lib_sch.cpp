#include "lib_sch.h"

parallel_scheduler::parallel_scheduler(int threads_num)
    : thread_count(threads_num)
{
    worker_threads = new pthread_t[thread_count];

    for (int i = 0; i < thread_count; ++i) {
        pthread_create(&worker_threads[i], nullptr, thread_entry, this);
    }
}

parallel_scheduler::~parallel_scheduler() {
    pthread_mutex_lock(&lock);
    terminate_flag = 1;
    pthread_mutex_unlock(&lock);

    pthread_cond_broadcast(&cond_task);

    for (int i = 0; i < thread_count; ++i) {
        pthread_join(worker_threads[i], nullptr);
    }

    delete[] worker_threads;

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond_task);
}

void* parallel_scheduler::thread_entry(void* ctx) {
    auto* self = static_cast<parallel_scheduler*>(ctx);

    while (true) {
        pthread_mutex_lock(&self->lock);

        while (self->task_queue.empty() && !self->terminate_flag) {
            pthread_cond_wait(&self->cond_task, &self->lock);
        }

        if (self->task_queue.empty() && self->terminate_flag) {
            pthread_mutex_unlock(&self->lock);
            return nullptr;
        }

        work_item* item = self->task_queue.front();
        self->task_queue.pop();

        pthread_mutex_unlock(&self->lock);

        item->job();
        delete item;
    }
}

void parallel_scheduler::run(void (*fn)(int), int value) {
    auto* job_item = new work_item;
    job_item->job = [fn, value]() { fn(value); };

    pthread_mutex_lock(&lock);
    task_queue.push(job_item);

    pthread_cond_signal(&cond_task);
    pthread_mutex_unlock(&lock);
}
