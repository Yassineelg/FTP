#pragma once

#include <queue>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "thread_pool.hpp"

class ClientQueueThreadPool : public ThreadPool {
public:
    ClientQueueThreadPool(size_t numThreads);
    ~ClientQueueThreadPool();

    void stop();
    void enqueueClientTask(int clientSocketFd, std::function<void()> task);

private:
    struct ClientResources {
        std::queue<std::function<void()>> taskQueue;
        std::unique_ptr<std::condition_variable> conditionVar;
        std::unique_ptr<std::thread> processingThread;
    };

    void processClientQueue(int clientSocketFd);
    void startClientProcessing(int clientSocketFd);

    std::mutex clientMutex_;
    std::unordered_map<int, ClientResources> clientResources_;
    bool stopFlag_;
};
