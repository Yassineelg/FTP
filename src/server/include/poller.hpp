#pragma once

#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <functional>

#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#elif _WIN32
#include <windows.h>
#endif

class Poller {
public:
    Poller();
    ~Poller();
    bool add(int fd);
    bool remove(int fd);
    void wait(std::function<void(int)> onEvent);

private:
#ifdef __linux__
    int epoll_fd;
#elif __APPLE__
    int kqueue_fd;
#elif _WIN32
    HANDLE iocp;
#endif
};

