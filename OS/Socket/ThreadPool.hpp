#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace SimpleNet {

class ThreadPool {
public:
    ThreadPool(size_t threads);
    void enqueue(std::function<void()> task);
    ~ThreadPool();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex mutex_;
    std::condition_variable condition_;
    bool stop_ = false;
};

}
