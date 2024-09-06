#ifndef CLI_HPP
#define CLI_HPP

#include "core/FTPClient.hpp"
#include "service/Logger.hpp"

class CLI {
public:
    explicit CLI(FTPClient* ftpClient);
    void start() const;

private:
    FTPClient* ftpClient;

    void connectToServer() const;
    void loginToServer() const;
    void processCommands() const;
    static void showPrompt();
    static std::string getUserInput(const std::string& prompt, const std::string& defaultValue = "");
};

#endif
