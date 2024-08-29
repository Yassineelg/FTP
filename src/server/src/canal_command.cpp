#include "../include/canal_command.hpp"
#include <cerrno>
#include <cstring>
#include <arpa/inet.h>

CanalCommand::CanalCommand(int port, ClientQueueThreadPool* queueClient)
    : queueClient_(queueClient) {
    setupServer(port);

//Login
    commandHandlers_["USER"] = &CanalCommand::handleUserCommand;
    commandHandlers_["PASS"] = &CanalCommand::handlePassCommand;
//Transfer
    commandHandlers_["STOR"] = &CanalCommand::handleStorCommand;
    commandHandlers_["RETR"] = &CanalCommand::handleRetrCommand;
//File action
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

bool CanalCommand::handleClient(FTPClient* client) {
    char buffer[1024];
    ssize_t bytesRead = read(client->socket_fd, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string command(buffer);
        processCommand(client, command);
        return true;
    } else if (bytesRead == 0) {
        std::cout << "Client déconnecté proprement, socket " << client->socket_fd << std::endl;
        return false;
    } else {
        std::cerr << "Erreur de lecture du socket " << client->socket_fd << ": " << std::strerror(errno) << std::endl;
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

void CanalCommand::processCommand(FTPClient* client, const std::string& command) {
    this->client = client;
    std::string commandKey = command.substr(0, 4);
    bool isAuthenticated = client->authenticated;

    bool isCommandAllowed = (isAuthenticated && commandKey != "USER" && commandKey != "PASS") || (!isAuthenticated && (commandKey == "USER" || commandKey == "PASS"));
    if (isCommandAllowed) {
        queueClient_->enqueueClientTask(client->socket_fd, [this, client, commandKey, command]() {    
            auto it = commandHandlers_.find(commandKey);
            if (it != commandHandlers_.end()) {
                    (this->*(it->second))(client->socket_fd);
            } else {
                std::cout << "500 Syntax error, command unrecognized. \"" << command << '"' << std::endl;
                sendToClient(client->socket_fd, "500 Syntax error, command unrecognized.");
            }
        });
    } else {
        if (isAuthenticated && (commandKey == "USER" || commandKey == "PASS")) {
            std::cout << "530 Bad sequence of commands. Already Logged \"" << command << '"' << std::endl;
            sendToClient(client->socket_fd, "530 Bad sequence of commands.");
        } else {
            std::cout << "530 Not logged in or command not allowed. \"" << command << '"' << std::endl;
            sendToClient(client->socket_fd, "530 Not logged in or command not allowed.");
        }
    }
}

//Login, Logout
void CanalCommand::handleUserCommand(int clientSocket) {
    if (client->username.empty()) {
        std::cout << "Socket: [" << clientSocket << "], Command: USER" << std::endl;
        client->username = "test";
        sendToClient(clientSocket, "331 User name okay, need password.");
    } else {
        sendToClient(clientSocket, "503 Bad sequence of commands.");
    }
}

void CanalCommand::handlePassCommand(int clientSocket) {

    if (client->username.empty()) {
        sendToClient(clientSocket, "503 Bad sequence of commands.");
        return;
    }
    if (true) { // gestion password et correct
        std::cout << "Socket: [" << clientSocket << "], Command: PASS" << std::endl;
        client->authenticated = true;
        sendToClient(clientSocket, "230 User logged in, proceed.");
    } else {
        sendToClient(clientSocket, "530 Not logged in.");
    }
}

void CanalCommand::handleQuitCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: QUIT " << std::endl;
    sendToClient(clientSocket, "250 Command okay.");
}

//File action
void CanalCommand::handleListCommand(int clientSocket) {
    std::cout << "Socket: ["  << clientSocket << "], Command: LIST " << std::endl;
    sendToClient(clientSocket, "250 Command okay.");
}

//Transfer
void CanalCommand::handleStorCommand(int clientSocket) {
    std::cout << "Socket: [" << clientSocket << "], Command: STOR " << std::endl;

    queueClient_->enqueueClientTask(clientSocket, [this, clientSocket]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "STOR operation completed for socket " << clientSocket << std::endl;
        sendToClient(clientSocket, "226 Transfer complete");
    });
    sendToClient(clientSocket, "150 File status okay; about to open data connection.");
}

void CanalCommand::handleRetrCommand(int clientSocket) {
    std::cout << "Socket: [" << clientSocket << "], Command: RETR " << std::endl;

    queueClient_->enqueueClientTask(clientSocket, [this, clientSocket]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "RETR operation completed for socket " << clientSocket << std::endl;
        sendToClient(clientSocket, "226 Transfer complete");
    });
    sendToClient(clientSocket, "150 File status okay; about to open data connection.");
}
