#include "../include/server_ftp.hpp"

ServerFTP::ServerFTP(int portCommand) : queueClient_(MAX_THREAD), commandServer_(portCommand, &queueClient_), poller_() { }

ServerFTP::~ServerFTP() {
    for (const auto& client : clients) {
        if (client.socket_fd >= 0) {
            close(client.socket_fd);
        }
    }
    close(commandServer_.getServerSocket());
}

void ServerFTP::run() {
    poller_.add(commandServer_.getServerSocket());

    while (true) {
        poller_.wait([this](int fd) {
            if (fd == commandServer_.getServerSocket()) {
                handleNewConnection();
            } else {
                handleClientData(fd);
            }
        });
    }
}

void ServerFTP::handleNewConnection() {
    int clientSocket = commandServer_.acceptClient();

    if (clientSocket >= 0) {
        clients.emplace_back(clientSocket);
        poller_.add(clientSocket);
        std::cout << "Client connecté (socket: " << clientSocket << ")" << std::endl;
    }
}

void ServerFTP::handleClientData(int fd) {
    try {
        bool found = false;
        for (auto& client : clients) {
            if (client.socket_fd == fd) {
                found = true;
                if (!commandServer_.handleClient(&client)) {
                    handleClientDisconnection(fd);
                }
                break;
            }
        }

        if (!found) {
            std::cerr << "Client with socket " << fd << " not found in the vector" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        handleClientDisconnection(fd);
    }
}

void ServerFTP::handleClientDisconnection(int fd) {
    auto it = std::remove_if(clients.begin(), clients.end(), [fd](const FTPClient& client) {
        return client.socket_fd == fd;
    });

    if (it != clients.end()) {
        close(fd);
        clients.erase(it, clients.end());
        poller_.remove(fd);
    }
}
