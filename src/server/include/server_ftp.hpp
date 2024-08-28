#pragma once

#include "server_command.hpp"
#include "poller.hpp"

#include <map>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <cstring>

class ServerFTP {
public:
    explicit ServerFTP(int portCommand = 21);
    ~ServerFTP();
    void run();

private:
    ServerCommand commandServer_;
    Poller poller_;
    std::map<int, bool> clientSockets;

};
