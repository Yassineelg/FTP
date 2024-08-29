#pragma once

#include "canal_command.hpp"
#include "poller.hpp"
#include "ftp_client.hpp"

#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <future>

class ServerFTP {
public:
    explicit ServerFTP(int portCommand = 21);
    ~ServerFTP();
    void run();

private:
    CanalCommand commandServer_;
    Poller poller_;
    ClientQueueThreadPool queueClient_;
    std::vector<FTPClient> clients;

    void handleNewConnection();
    void handleClientData(int fd);
    void handleClientDisconnection(int fd);
};
