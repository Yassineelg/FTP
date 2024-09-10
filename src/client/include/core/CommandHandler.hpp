#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <unordered_map>
#include <functional>
#include <string>
#include <vector>
#include <utility>
#include "FTPClient.hpp"

class CommandHandler {
public:
    explicit CommandHandler(FTPClient* ftpClient);
    void handleCommand(const std::string& command, const std::string& args);

private:
    FTPClient* ftpClient;
    std::unordered_map<std::string, std::function<void(const std::string&)>> commandMap;

    void handlePort(const std::string& args);
    void handlePasv();

    std::vector<int> parsePortArgs(const std::string& args);
    void validatePortArgs(const std::vector<int>& parts);
    void validatePort(int port);
    std::string formatPortCommand(const std::vector<int>& parts);
    std::pair<std::string, int> parsePasvResponse(const std::string& response);
};

#endif // COMMAND_HANDLER_HPP
