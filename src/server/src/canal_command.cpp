#include "../include/canal_command.hpp"
#include <cerrno>
#include <cstring>
#include <arpa/inet.h>

CanalCommand::CanalCommand(int port) {
    setupServer(port);

//Login
    commandHandlers_["USER"] = &CanalCommand::handleUserCommand;
    commandHandlers_["PASS"] = &CanalCommand::handlePassCommand;
//Transfer parameters
    commandHandlers_["STOR"] = &CanalCommand::handleStorCommand;
    commandHandlers_["RETR"] = &CanalCommand::handleRetrCommand;
//File action commands
    commandHandlers_["LIST"] = &CanalCommand::handleListCommand;
//Logout
    commandHandlers_["QUIT"] = &CanalCommand::handleQuitCommand;
}

void CanalCommand::setupServer(int port) {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Erreur de création du socket: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_addr.s_addr = INADDR_ANY;
    serverAddr_.sin_port = htons(port);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr_, sizeof(serverAddr_)) < 0) {
        std::cerr << "Erreur de liaison du socket: " << std::strerror(errno) << std::endl;
        close(serverSocket_);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket_, 5) < 0) {
        std::cerr << "Erreur d'écoute du socket: " << std::strerror(errno) << std::endl;
        close(serverSocket_);
        exit(EXIT_FAILURE);
    }

    std::cout << "Serveur FTP, Port : " << port << std::endl;
}

void CanalCommand::sendToClient(int clientSocket, const std::string& message) {
    ssize_t bytesSent = write(clientSocket, message.c_str(), message.size());
    if (bytesSent < 0) {
        std::cerr << "Erreur d'écriture sur le socket " << clientSocket << ": " << std::strerror(errno) << std::endl;
    }
}

bool CanalCommand::handleClient(int clientSocket) {
    char buffer[1024];
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string command(buffer);
        processCommand(clientSocket, command, FTPMode::Passif);
        return true;
    } else if (bytesRead == 0) {
        std::cout << "Client déconnecté proprement, socket " << clientSocket << std::endl;
        return false;
    } else {
        std::cerr << "Erreur de lecture du socket " << clientSocket << ": " << std::strerror(errno) << std::endl;
        return false;
    }
}

int CanalCommand::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        std::cerr << "Erreur d'acceptation de la connexion: " << std::strerror(errno) << std::endl;
        return -1;
    }

    std::string clientIP = inet_ntoa(clientAddr.sin_addr);
    std::cout << "Client connecté (IP: " << clientIP << ", socket: " << clientSocket << ")" << std::endl;
    return clientSocket;
}

int CanalCommand::getServerSocket() const {
    return serverSocket_;
}

void CanalCommand::processCommand(int clientSocket, const std::string& command , FTPMode isPassive) {
    std::string commandKey = command.substr(0, 4);

    auto it = commandHandlers_.find(commandKey);
    if (it != commandHandlers_.end()) {
        (this->*(it->second))(clientSocket);
        sendToClient(clientSocket, "pong");
    } else {
        std::cout << "500 Syntax error, command unrecognized. \"" << command << '"' << std::endl;
        sendToClient(clientSocket, "500 Syntax error, command unrecognized.");
    }
}

void CanalCommand::handleUserCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: USER" << std::endl;
}

void CanalCommand::handlePassCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: PASS " << std::endl;
}

void CanalCommand::handleStorCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: STOR " << std::endl;
}

void CanalCommand::handleRetrCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: RETR " << std::endl;
}

void CanalCommand::handleQuitCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: QUIT " << std::endl;
}

void CanalCommand::handleListCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: LIST " << std::endl;
}
