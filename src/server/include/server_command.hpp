#pragma once
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <map>

class ServerFTP;

enum class FTPMode {
    Actif,
    Passif
};

class ServerCommand {
public:
    ServerCommand(int port);
    int acceptClient();
    int getServerSocket() const;
    bool handleClient(int clientSocket);
    void sendToClient(int clientSocket, const std::string& message);

private:
    int serverSocket_;
    struct sockaddr_in serverAddr_;
    using CommandHandler = void (ServerCommand::*)(int);
    std::map<std::string, CommandHandler> commandHandlers_;

    void setupServer(int port);
    void processCommand(int clientSocket, const std::string& command, FTPMode isPassive);
    void handleUserCommand(int clientSocket);
    void handlePassCommand(int clientSocket);
    void handleStorCommand(int clientSocket);
    void handleRetrCommand(int clientSocket);
    void handleQuitCommand(int clientSocket);
    void handleListCommand(int clientSocket);
};

