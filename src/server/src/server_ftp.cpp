#include "../include/server_ftp.hpp"
#include "../include/poller.hpp"

ServerFTP::ServerFTP(int portCommand) : commandServer_(portCommand), poller_() {}

ServerFTP::~ServerFTP() {
    close(commandServer_.getServerSocket());
    for (const auto& [clientSocket, _] : clientSockets) {
        close(clientSocket);
    }
}

void ServerFTP::run() {
    poller_.add(commandServer_.getServerSocket());

    while (true) {
        poller_.wait([this](int fd) {
            if (fd == commandServer_.getServerSocket()) {
                int clientSocket = commandServer_.acceptClient();
                if (clientSocket >= 0) {
                    clientSockets[clientSocket] = false;
                    poller_.add(clientSocket);
                    std::cout << "Client connecté (socket: " << clientSocket << ")" << std::endl;
                }
            } else {
                try {
                    if (!commandServer_.handleClient(fd)) {
                        poller_.remove(fd);
                        close(fd);
                        clientSockets.erase(fd);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Exception: " << e.what() << std::endl;
                    poller_.remove(fd);
                    close(fd);
                    clientSockets.erase(fd);
                }
            }
        });
    }
}
