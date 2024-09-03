#pragma once

#include "../include/thread_pool.hpp"
#include <map>
#include <queue>
#include <mutex>
#include <functional>
#include <thread>
#include <condition_variable>
#include <iostream> 

class ClientQueueThreadPool : public ThreadPool {
public:
    ClientQueueThreadPool(size_t numThreads);
    ~ClientQueueThreadPool();
    void enqueueClientTask(int clientSocketFd, std::function<void()> task);
 
private:
    std::map<int, std::queue<std::function<void()>>> clientQueues;
    std::map<int, std::condition_variable> clientCVs;
    std::map<int, std::mutex> clientMutexes;
    std::map<int, std::thread> clientThreads;
    std::map<int, bool> clientProcessing;

    std::mutex queueMutex;

    void processClientQueue(int clientSocketFd);
};
