#ifndef CLI_HPP
#define CLI_HPP

#include "core/FTPClient.hpp"
#include "core/CommandHandler.hpp"

class CLI {
public:
    explicit CLI(FTPClient* ftpClient, CommandHandler* commandHandler);
    void start() const;

private:
    FTPClient* ftpClient;
    CommandHandler* commandHandler;

    void connectToServer() const;
    void loginToServer() const;
    void processCommands() const;
    static void showPrompt();
    static std::string getUserInput(const std::string& prompt, const std::string& defaultValue = "");
};

#endif // CLI_HPP
