#include "../include/poller.hpp"
#include <cerrno>
#include <cstring>
#include <iostream>

Poller::Poller() {
#ifdef __linux__
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Erreur lors de la création de epoll: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
#elif __APPLE__
    kqueue_fd = kqueue();
    if (kqueue_fd == -1) {
        std::cerr << "Erreur lors de la création de kqueue: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
#elif _WIN32
    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (iocp == NULL) {
        std::cerr << "Erreur lors de la création de IOCP: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

Poller::~Poller() {
#ifdef __linux__
    close(epoll_fd);
#elif __APPLE__
    close(kqueue_fd);
#elif _WIN32
    CloseHandle(iocp);
#endif
}

bool Poller::add(int fd) {
#ifdef __linux__
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == 0;
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    return kevent(kqueue_fd, &ev, 1, NULL, 0, NULL) == 0;
#elif _WIN32
    HANDLE handle = (HANDLE)fd;
    return CreateIoCompletionPort(handle, iocp, (ULONG_PTR)fd, 0) != NULL;
#else
    return false;
#endif
}

bool Poller::remove(int fd) {
#ifdef __linux__
    return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == 0;
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    return kevent(kqueue_fd, &ev, 1, NULL, 0, NULL) == 0;
#elif _WIN32
    // Pour IOCP, la suppression se fait généralement en fermant le socket
    return true;
#else
    return false;
#endif
}

void Poller::wait(std::function<void(int)> onEvent) {
#ifdef __linux__
    epoll_event events[10];
    int nfds = epoll_wait(epoll_fd, events, 10, -1);
    for (int i = 0; i < nfds; ++i) {
        onEvent(events[i].data.fd);
    }
#elif __APPLE__
    struct kevent events[10];
    int nev = kevent(kqueue_fd, NULL, 0, events, 10, NULL);
    for (int i = 0; i < nev; ++i) {
        onEvent(events[i].ident);
    }
#elif _WIN32
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped;
    while (GetQueuedCompletionStatus(iocp, &bytesTransferred, &completionKey, &overlapped, INFINITE)) {
        onEvent(static_cast<int>(completionKey));
    }
#endif
}