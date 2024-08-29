#include "../include/client_queue_thread_pool.hpp"

ClientQueueThreadPool::ClientQueueThreadPool(size_t numThreads)
    : ThreadPool(numThreads) { }

ClientQueueThreadPool::~ClientQueueThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        for (auto& [clientSocketFd, thread] : clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
}

void ClientQueueThreadPool::enqueueClientTask(int clientSocketFd, std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        clientQueues[clientSocketFd].emplace(std::move(task));
        if (clientThreads.find(clientSocketFd) == clientThreads.end()) {
            clientThreads[clientSocketFd] = std::thread(&ClientQueueThreadPool::processClientQueue, this, clientSocketFd);
        }
    }
    clientCVs[clientSocketFd].notify_one();
}

void ClientQueueThreadPool::processClientQueue(int clientSocketFd) {

    while (true) {
        std::unique_lock<std::mutex> clientLock(clientMutexes[clientSocketFd]);
        clientCVs[clientSocketFd].wait(clientLock, [this, clientSocketFd] {
            bool hasTasks = !clientQueues[clientSocketFd].empty();
            return hasTasks;
        });

        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            auto& queue = clientQueues[clientSocketFd];
            if (queue.empty()) {
                return;
            }

            task = std::move(queue.front());
            queue.pop();
        }

        enqueue([this, clientSocketFd, task]() {
            std::cout << "Executing task for client " << clientSocketFd << "\n";
            task();
            std::cout << "Finished executing task for client " << clientSocketFd << "\n";
        });
    }
}
