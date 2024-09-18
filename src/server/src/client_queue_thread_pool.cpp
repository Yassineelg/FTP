#include "client_queue_thread_pool.hpp"

ClientQueueThreadPool::ClientQueueThreadPool(size_t numThreads)
    : ThreadPool(numThreads), stopFlag_(false) {
}

ClientQueueThreadPool::~ClientQueueThreadPool() {
    stop();
    {
        std::unique_lock<std::mutex> lock(clientMutex_);
        for (auto& kv : clientResources_) {
            auto& resources = kv.second;
            if (resources.processingThread && resources.processingThread->joinable()) {
                resources.processingThread->join();
            }
        }
    }
}

void ClientQueueThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(clientMutex_);
        stopFlag_ = true;
    }

    for (auto& kv : clientResources_) {
        if (kv.second.conditionVar) {
            kv.second.conditionVar->notify_all();
        }
    }

    ThreadPool::stop();
}

void ClientQueueThreadPool::enqueueClientTask(int clientSocketFd, std::function<void()> task) {
    if (!task) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(clientMutex_);
        if (stopFlag_) {
            return;
        }

        auto& resources = clientResources_[clientSocketFd];
        if (!resources.conditionVar) {
            resources.conditionVar = std::make_unique<std::condition_variable>();
            startClientProcessing(clientSocketFd);
        }

        resources.taskQueue.emplace(std::move(task));
    }
    if (clientResources_[clientSocketFd].conditionVar) {
        clientResources_[clientSocketFd].conditionVar->notify_one();
    }
}

void ClientQueueThreadPool::processClientQueue(int clientSocketFd) {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(clientMutex_);
            auto it = clientResources_.find(clientSocketFd);
            if (it == clientResources_.end()) {
                return;
            }

            auto& resources = it->second;
            if (!resources.conditionVar) {
                return;
            }

            resources.conditionVar->wait(lock, [this, clientSocketFd] {
                return stopFlag_ || !clientResources_[clientSocketFd].taskQueue.empty();
            });

            if (stopFlag_ && clientResources_[clientSocketFd].taskQueue.empty()) {
                return;
            }

            if (!clientResources_[clientSocketFd].taskQueue.empty()) {
                task = std::move(clientResources_[clientSocketFd].taskQueue.front());
                clientResources_[clientSocketFd].taskQueue.pop();
            }
        }

        if (task) {
            task();
        }
    }
}

void ClientQueueThreadPool::startClientProcessing(int clientSocketFd) {
    auto& resources = clientResources_[clientSocketFd];
    resources.processingThread = std::make_unique<std::thread>(&ClientQueueThreadPool::processClientQueue, this, clientSocketFd);
}
