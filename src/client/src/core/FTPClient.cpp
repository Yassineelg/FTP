#include "./../../include/core/FTPClient.hpp"
#include "./../../include/service/Logger.hpp"

FTPClient::FTPClient(NetworkService* networkService) : networkService(networkService) {}

bool FTPClient::connect(const std::string& host, const int port) const {
    return networkService->connectToServer(host, port);
}

bool FTPClient::login(const std::string& username, const std::string& password) const {
    std::string response = sendCommand("USER", username);
    Logger::debug("Response to USER: " + response);

    if (response.find("331") == std::string::npos) {
        Logger::error("USER command failed: " + response);
        return false;
    }

    response = sendCommand("PASS", password);
    Logger::debug("Response to PASS: " + response);

    if (response.find("230") == std::string::npos) {
        Logger::error("PASS command failed: " + response);
        return false;
    }

    return true;
}

std::string FTPClient::sendCommand(const std::string& command, const std::string& args) const {
    std::string fullCommand = command;
    if (!args.empty()) {
        fullCommand += " " + args;
    }
    fullCommand += "\r\n";

    const std::string response = networkService->sendAndReceive(fullCommand);
    return sanitizeResponse(response);
}

void FTPClient::close() const {
    networkService->closeConnection();
}

std::string FTPClient::sanitizeResponse(const std::string& response) {
    std::string sanitized = response;
    std::erase(sanitized, '\r');
    std::erase(sanitized, '\n');
    return sanitized;
}
