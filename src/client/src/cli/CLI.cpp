#include "./../../include/cli/CLI.hpp"
#include "./../../include/core/CommandHandler.hpp"
#include "./../../include/service/Logger.hpp"
#include "./../../include/Color.hpp"
#include "./../../include/config.hpp"
#include <iostream>

CLI::CLI(FTPClient* ftpClient, CommandHandler* commandHandler)
    : ftpClient(ftpClient), commandHandler(commandHandler) {}

void CLI::start() const {
    connectToServer();
    loginToServer();
    processCommands();
}

void CLI::connectToServer() const {
    bool connected = false;
    while (!connected) {
        std::string host = getUserInput("Enter FTP server host", "");
        std::string portStr = getUserInput("Enter port (default 21)", std::to_string(DEFAULT_FTP_PORT));

        int port;
        try { port = std::stoi(portStr); }
        catch (const std::exception&) {
            Logger::error("Invalid port number.");
            continue;
        }

        if (ftpClient->connect(host, port)) {
            Logger::info("Connected to server.");
            connected = true;
        }
        else { Logger::error("Failed to connect to server. Retrying..."); }
    }
}

void CLI::loginToServer() const {
    bool loggedIn = false;
    while (!loggedIn) {
        std::string username = getUserInput("Enter username", "");
        std::string password = getUserInput("Enter password", "");

        if (ftpClient->login(username, password)) {
            Logger::info("Logged in successfully.");
            loggedIn = true;
        }
        else { Logger::error("Login failed. Retrying..."); }
    }
}

void CLI::processCommands() const {
    std::string command;

    while (true) {
        showPrompt();
        std::getline(std::cin, command);

        if (command.empty()) {
            Logger::warn("Empty command entered.");
            continue;
        }

        if (command == "QUIT") {
            ftpClient->close();
            Logger::info("Disconnected from server.");
            break;
        }

        const size_t spacePos = command.find(' ');
        std::string cmd = (spacePos == std::string::npos) ? command : command.substr(0, spacePos);
        std::string args = (spacePos == std::string::npos) ? "" : command.substr(spacePos + 1);

        commandHandler->handleCommand(cmd, args);
    }
}

void CLI::showPrompt() { std::cout << CYAN << "ftp> " << RESET; }

std::string CLI::getUserInput(const std::string& prompt, const std::string& defaultValue) {
    std::string input;
    std::cout << CYAN << prompt << ": " << RESET;
    std::getline(std::cin, input);
    return input.empty() ? defaultValue : input;
}
