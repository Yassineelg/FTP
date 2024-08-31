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
#include <regex>
#include <iterator>

class ServerFTP;

class CanalCommand {
public:
    CanalCommand(int port, ClientQueueThreadPool* threadPool);
    int acceptClient();
    int getServerSocket() const;
    bool handleClient(FTPClient* client);
    void sendToClient(int clientSocket, const std::string& message);

private:
    int serverSocket_command;
    struct sockaddr_in serverAddr_command;

    using CommandHandler = void (CanalCommand::*)(FTPClient *, std::vector<std::string>);
    std::map<std::string, CommandHandler> commandHandlers_;

    ClientQueueThreadPool* queueClient_;
    FTPClient* client;

    void setupServer(int port);
    void processCommand(FTPClient* client, const std::string& command);
    void handleUserCommand(FTPClient* client, std::vector<std::string> command);
    void handlePassCommand(FTPClient* client, std::vector<std::string> command);
    void handleStorCommand(FTPClient* client, std::vector<std::string> command);
    void handleRetrCommand(FTPClient* client, std::vector<std::string> command);
    void handleQuitCommand(FTPClient* client, std::vector<std::string> command);
    void handleListCommand(FTPClient* client, std::vector<std::string> command);
};
