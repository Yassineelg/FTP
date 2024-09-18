#pragma once

#include "ftp_client.hpp"
#include "thread_pool.hpp"
#include "client_queue_thread_pool.hpp"
#include "configserver.hpp"
#include "sslconnect.hpp"

#include "canal_data.hpp"
#include "define.hpp"
#include "configserver.hpp"

#include <map>
#include <vector>
#include <regex>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <sys/utsname.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

class ServerFTP;
class ConfigServer;

class CanalCommand {
public:
    CanalCommand(ConfigServer *configServ, ClientQueueThreadPool* threadPool = nullptr);
    ~CanalCommand() {};

    int acceptClient();
    int getServerSocket() const;
    bool handleClient(FTPClient* client);
    void sendToClient(FTPClient *client, const std::string& message);
    std::string getTime();

    bool serverStart_;
    void setLogClient(std::string message);

private:
    void setupServer(int port);
    void processCommand(FTPClient* client, const std::string& command);
    int createAvailablePort();

    void handleAuthCommand(FTPClient *client, std::vector<std::string> commands);
    void handleUserCommand(FTPClient* client, std::vector<std::string> command);
    void handlePassCommand(FTPClient* client, std::vector<std::string> command);
    void handleQuitCommand(FTPClient* client, std::vector<std::string> command);
    void handlePortCommand(FTPClient* client, std::vector<std::string> command);
    void handlePasvCommand(FTPClient* client, std::vector<std::string> command);
    void handlePwdCommand(FTPClient* client, std::vector<std::string> command);
    void handleMkdCommand(FTPClient* client, std::vector<std::string> command);
    void handleCwdCommand(FTPClient* client, std::vector<std::string> command);
    void handleRmdCommand(FTPClient* client, std::vector<std::string> command);
    void handleDeleCommand(FTPClient* client, std::vector<std::string> command);
    void handleStorCommand(FTPClient* client, std::vector<std::string> command);
    void handleRetrCommand(FTPClient* client, std::vector<std::string> command);
    void handleNlstCommand(FTPClient* client, std::vector<std::string> command);
    void handleTypeCommand(FTPClient* client, std::vector<std::string> command);
    void handleListCommand(FTPClient* client, std::vector<std::string> command);
    void handleCdupCommand(FTPClient* client, std::vector<std::string> command);
    void handleReinCommand(FTPClient* client, std::vector<std::string> command);
    void handlePbszCommand(FTPClient* client, std::vector<std::string> command);
    void handleProtCommand(FTPClient* client, std::vector<std::string> command);
    void handleMdtmCommand(FTPClient* client, std::vector<std::string> command);
    void handleSizeCommand(FTPClient* client, std::vector<std::string> command);
    void handleNoopCommand(FTPClient* client, std::vector<std::string> command);
    void handleAlloCommand(FTPClient* client, std::vector<std::string> command);
    void handleSystCommand(FTPClient* client, std::vector<std::string> command);
    void handleStatCommand(FTPClient* client, std::vector<std::string> command);
    void handleFeatCommand(FTPClient* client, std::vector<std::string> command);

    ConfigServer *configServer_;

    SslConnect sslConnect_;

    int serverSocketCommand_;
    struct sockaddr_in serverAddrCommand_;

    using CommandHandler = void (CanalCommand::*)(FTPClient *, std::vector<std::string>);
    std::map<std::string, CommandHandler> commandHandlers_;

    ClientQueueThreadPool* queueClient_;
    FTPClient* client_;

    std::vector<int> portUse_;

};
