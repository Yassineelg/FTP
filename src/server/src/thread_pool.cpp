#include "thread_pool.hpp"

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    condition_.wait(lock, [this] {
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    if (!tasks_.empty()) {
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                }
                if (task) {
                    task();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop();
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    if (!task) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (stop_) {
            return;
        }
        tasks_.emplace(std::move(task));
    }
    condition_.notify_one();
}

void ThreadPool::stop() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    stop_ = true;
    condition_.notify_all();
}
