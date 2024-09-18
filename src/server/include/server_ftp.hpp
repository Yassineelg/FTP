#pragma once

#include "define.hpp"

#include "canal_command.hpp"
#include "poller.hpp"
#include "ftp_client.hpp"
#include "client_queue_thread_pool.hpp"

#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <future>
#include <sys/stat.h>
#include <errno.h>
#include <chrono>

#include "configserver.hpp"

class ConfigServer;
class CanalCommand;

class ServerFTP {
public:
    ServerFTP(ConfigServer *configServ, int portCommand = 21);
    ~ServerFTP();

    void run();
    void stop();

private:
    void closeAll();
    void handleNewConnection();
    void handleClientData(int fd);
    void handleClientDisconnection(int fd);

    ConfigServer *configServ_;

    bool serverStart_;
    std::vector<FTPClient> clients_;
    Poller *poller_;
    ClientQueueThreadPool *queueClient_;
    CanalCommand *commandServer_;
    int port_;
    int clientConnected_;
};

