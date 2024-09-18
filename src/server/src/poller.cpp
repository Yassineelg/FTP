#include "poller.hpp"

Poller::Poller() {
#ifdef __linux__
    epollFd_ = epoll_create1(0);
    if (epollFd_ == -1) {
        std::cerr << "Erreur lors de la création de epoll: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
#elif __APPLE__
    kqueueFd_ = kqueue();
    if (kqueueFd_ == -1) {
        std::cerr << "Erreur lors de la création de kqueue: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

Poller::~Poller() {
#ifdef __linux__
    close(epollFd_);
#elif __APPLE__
    close(kqueueFd_);
#endif
}

bool Poller::add(int fd) {
#ifdef __linux__
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    return epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == 0;
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    return kevent(kqueueFd_, &ev, 1, NULL, 0, NULL) == 0;
#else
    return false;
#endif
}

bool Poller::remove(int fd) {
#ifdef __linux__
    return epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == 0;
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    return kevent(kqueueFd_, &ev, 1, NULL, 0, NULL) == 0;
#else
    return false;
#endif
}

void Poller::wait(std::function<void(int)> onEvent, std::chrono::milliseconds timeout) {
#ifdef __linux__
    epoll_event events[10];
    int nfds = epoll_wait(epollFd_, events, 10, timeout.count());
    for (int i = 0; i < nfds; ++i) {
        onEvent(events[i].data.fd);
    }
#elif __APPLE__
    struct kevent events[10];
    struct timespec ts;
    ts.tv_sec = timeout.count() / 1000;
    ts.tv_nsec = (timeout.count() % 1000) * 1000000;
    int nev = kevent(kqueueFd_, NULL, 0, events, 10, &ts);
    for (int i = 0; i < nev; ++i) {
        onEvent(events[i].ident);
    }
#endif
}
