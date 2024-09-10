#include "./../../include/core/CommandHandler.hpp"
#include "./../../include/service/Logger.hpp"
#include <sstream>
#include <regex>
#include <stdexcept>

CommandHandler::CommandHandler(FTPClient* ftpClient) : ftpClient(ftpClient) {
    commandMap["PORT"] = [this](const std::string& args) { handlePort(args); };
    commandMap["PASV"] = [this](const std::string&) { handlePasv(); };
}

void CommandHandler::handleCommand(const std::string& command, const std::string& args) {
    auto it = commandMap.find(command);
    if (it != commandMap.end()) {
        it->second(args);
    } else {
        std::string response = ftpClient->sendCommand(command + " " + args);
        Logger::info(response);
    }
}

void CommandHandler::handlePort(const std::string& args) {
    try {
        auto parts = parsePortArgs(args);
        validatePortArgs(parts);

        std::string portCommand = formatPortCommand(parts);

        std::string response = ftpClient->sendCommand(portCommand);
        Logger::info(response);
    } catch (const std::exception& e) {
        Logger::error(e.what());
    }
}

void CommandHandler::handlePasv() {
    try {
        std::string response = ftpClient->sendCommand("PASV");
        Logger::info(response);

        auto [ip, port] = parsePasvResponse(response);
        validatePort(port);

        Logger::info("Passive mode IP: " + ip + ", Port: " + std::to_string(port));
    } catch (const std::exception& e) {
        Logger::error(e.what());
    }
}

std::vector<int> CommandHandler::parsePortArgs(const std::string& args) {
    std::istringstream iss(args);
    std::string token;
    std::vector<int> parts;

    while (std::getline(iss, token, ',')) {
        parts.push_back(std::stoi(token));
    }

    if (parts.size() != 6) {
        throw std::invalid_argument("Invalid arguments. Usage: PORT <h1,h2,h3,h4,p1,p2>");
    }

    return parts;
}

void CommandHandler::validatePortArgs(const std::vector<int>& parts) {
    for (int i = 0; i < 4; ++i) {
        if (parts[i] < 0 || parts[i] > 255) {
            throw std::invalid_argument("Invalid IP address format.");
        }
    }

    for (int i = 4; i < 6; ++i) {
        if (parts[i] < 0 || parts[i] > 255) {
            throw std::invalid_argument("Invalid port number format.");
        }
    }

    int port = parts[4] * 256 + parts[5];
    validatePort(port);
}

void CommandHandler::validatePort(int port) {
    if (port < 1 || port > 65535) {
        throw std::invalid_argument("Invalid port number. Must be between 1 and 65535.");
    }
}

std::string CommandHandler::formatPortCommand(const std::vector<int>& parts) {
    return "PORT " + std::to_string(parts[0]) + "," + std::to_string(parts[1]) + "," +
           std::to_string(parts[2]) + "," + std::to_string(parts[3]) + "," +
           std::to_string(parts[4]) + "," + std::to_string(parts[5]);
}

std::pair<std::string, int> CommandHandler::parsePasvResponse(const std::string& response) {
    std::regex pasvRegex(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
    std::smatch match;

    if (std::regex_search(response, match, pasvRegex) && match.size() == 7) {
        std::string ip = match[1].str() + "." + match[2].str() + "." + match[3].str() + "." + match[4].str();
        int port = std::stoi(match[5].str()) * 256 + std::stoi(match[6].str());
        return {ip, port};
    } else {
        throw std::runtime_error("Failed to parse PASV response.");
    }
}
