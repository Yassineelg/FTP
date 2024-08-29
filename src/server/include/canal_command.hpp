#pragma once

#include "ftp_client.hpp"
#include "thread_pool.hpp"
#include "client_queue_thread_pool.hpp"

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <map>

class ServerFTP;

class CanalCommand {
public:
    CanalCommand(int port, ClientQueueThreadPool* threadPool);
    int acceptClient();
    int getServerSocket() const;
    bool handleClient(FTPClient* client);
    void sendToClient(int clientSocket, const std::string& message);

private:
    int serverSocket_;
    struct sockaddr_in serverAddr_;
    using CommandHandler = void (CanalCommand::*)(int);
    std::map<std::string, CommandHandler> commandHandlers_;
    ClientQueueThreadPool* queueClient_;
    FTPClient* client;

    void setupServer(int port);
    void processCommand(FTPClient* client, const std::string& command);
    void handleUserCommand(int clientSocket);
    void handlePassCommand(int clientSocket);
    void handleStorCommand(int clientSocket);
    void handleRetrCommand(int clientSocket);
    void handleQuitCommand(int clientSocket);
    void handleListCommand(int clientSocket);
};
