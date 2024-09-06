#include "./../../include/service/NetworkService.hpp"
#include "./../../include/service/Logger.hpp"
#include "./../../include/config.hpp"
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

NetworkService::NetworkService() : socketFd(-1) {}

NetworkService::~NetworkService() { closeConnection(); }

bool NetworkService::connectToServer(const std::string& host, const int port) {
    addrinfo hints{}, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    const std::string portStr = std::to_string(port);
    if (const int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        Logger::error("getaddrinfo: " + std::string(gai_strerror(status)));
        return false;
    }

    socketFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketFd < 0) {
        Logger::error("Could not create socket.");
        freeaddrinfo(res);
        return false;
    }

    if (::connect(socketFd, res->ai_addr, res->ai_addrlen) < 0) {
        Logger::error("Connection failed.");
        freeaddrinfo(res);
        return false;
    }

    freeaddrinfo(res);
    Logger::info("Connection established.");

    // Read and log the initial response from the server
    Logger::info("Initial server response: " + sanitizeResponse(readFromSocket()));

    return true;
}

void NetworkService::closeConnection() {
    if (socketFd >= 0) {
        ::close(socketFd);
        Logger::info("Socket closed.");
        socketFd = -1;
    }
}

std::string NetworkService::sendAndReceive(const std::string& data) const {
    writeToSocket(data);
    return readFromSocket();
}

void NetworkService::writeToSocket(const std::string& data) const {
    if (send(socketFd, data.c_str(), data.length(), 0) < 0) { Logger::error("Error writing to socket."); }
}

std::string NetworkService::readFromSocket() const {
    char buffer[MAX_BUFFER_SIZE];
    std::string response;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socketFd, &readfds);

    timeval timeout{};
    timeout.tv_sec = SOCKET_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    const int activity = select(socketFd + 1, &readfds, nullptr, nullptr, &timeout);

    if (activity < 0) {
        Logger::error("Select error.");
        return "";
    }
    else if (activity == 0) {
        Logger::error("Timeout occurred.");
        return "";
    }

    memset(buffer, 0, sizeof(buffer));
    const int bytesRead = recv(socketFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        Logger::error("Error reading from socket.");
        return "";
    }

    response += std::string(buffer);
    return response;
}

std::string NetworkService::sanitizeResponse(const std::string& response) {
    std::string sanitized = response;
    std::erase(sanitized, '\r');
    std::erase(sanitized, '\n');
    return sanitized;
}
