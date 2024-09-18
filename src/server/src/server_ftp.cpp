#include "server_ftp.hpp"

ServerFTP::ServerFTP(ConfigServer *configServ, int portCommand) {
    configServ_ = configServ;
    queueClient_  = new ClientQueueThreadPool(configServ_->getConfigValue<int>(configServ_->ConfigKey::MAX_CLIENTS_) * 10);
    poller_ = new Poller();
    port_ = portCommand;
    commandServer_ = nullptr;
}

ServerFTP::~ServerFTP() {
    closeAll();
}

void ServerFTP::stop() {
    queueClient_->stop();
    serverStart_ = false;
    std::ostringstream logMessage;
    logMessage << commandServer_->getTime() << " Stop Server" << "\n";
    std::cout << logMessage.str() << std::endl;
}

void ServerFTP::run() {
    serverStart_ = true;
    commandServer_ = new CanalCommand(configServ_, queueClient_);
    if (!commandServer_->serverStart_) {
        commandServer_->setLogClient("Port alredy in use.");
    }
    poller_->add(commandServer_->getServerSocket());

    while (serverStart_ && commandServer_->serverStart_) {
        poller_->wait([this](int fd) {
            if (fd == commandServer_->getServerSocket()) {
                handleNewConnection();
            } else {
                handleClientData(fd);
            }
        }, std::chrono::milliseconds(500));
    }
}

void ServerFTP::handleNewConnection() {
    int clientSocket = commandServer_->acceptClient();

    if (clientConnected_ > configServ_->getConfigValue<int>(configServ_->ConfigKey::MAX_CLIENTS_)) {
        close(clientSocket);
        return;
    }
    clientConnected_++;

    if (clientSocket >= 0) {
        clients_.emplace_back(clientSocket);
        poller_->add(clientSocket);

        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        memset(&clientAddr, 0, clientAddrLen);
        getpeername(clientSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
    }
}

void ServerFTP::handleClientData(int fd) {
    try {
        bool found = false;
        for (auto& client : clients_) {
            if (client.socket_fd == fd) {
                found = true;
                if (!commandServer_->handleClient(&client)) {
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
    auto it = std::remove_if(clients_.begin(), clients_.end(), [fd](const FTPClient& client) {
        return client.socket_fd == fd;
    });

    clientConnected_--;
    if (it != clients_.end()) {
        close(fd);
        clients_.erase(it, clients_.end());
        poller_->remove(fd);
    }
}

void ServerFTP::closeAll() {
    for (auto& client : clients_) {
        if (client.socket_fd >= 0) {
            close(client.socket_fd);
        }
    }

    clients_.clear();

    if (poller_) {
        poller_->remove(commandServer_->getServerSocket());
        delete poller_;
        poller_ = nullptr;
    }

    if (commandServer_) {
        close(commandServer_->getServerSocket());
        delete commandServer_;
        commandServer_ = nullptr;
    }

    if (queueClient_) {
        queueClient_->stop();
        delete queueClient_;
        queueClient_ = nullptr;
    }
}


