#include "canal_command.hpp"

CanalCommand::CanalCommand(ConfigServer *configServ, ClientQueueThreadPool* queueClient)
    : configServer_(configServ), queueClient_(queueClient) {

    setupServer(configServer_->getConfigValue<int>(configServer_->ConfigKey::PORT_));

    client_ = nullptr;
    if (configServer_->getConfigValue<std::string>(configServer_->ConfigKey::SSL_ACTIVATE_) == "true") {
        bool eSsl = true;
        if (!sslConnect_.initialize()) {
            eSsl = false;
        }
        std::string key_file(FTP_DEFAULT_DIR_SSL + "server.key");
        std::string cert_file(FTP_DEFAULT_DIR_SSL + "server.crt");
        if (!sslConnect_.configure(cert_file.c_str(), key_file.c_str())) {
            eSsl = false;
        }
        if (eSsl) {
        //FTPS, FTPES
            commandHandlers_["AUTH"] = &CanalCommand::handleAuthCommand;
            commandHandlers_["PBSZ"] = &CanalCommand::handlePbszCommand;
            commandHandlers_["PROT"] = &CanalCommand::handleProtCommand;
        }
    }

    // Login
    commandHandlers_["USER"] = &CanalCommand::handleUserCommand;
    commandHandlers_["PASS"] = &CanalCommand::handlePassCommand;
    commandHandlers_["REIN"] = &CanalCommand::handleReinCommand;
    // Transfer
    commandHandlers_["STOR"] = &CanalCommand::handleStorCommand;
    commandHandlers_["RETR"] = &CanalCommand::handleRetrCommand;
    commandHandlers_["ALLO"] = &CanalCommand::handleAlloCommand;
    // System
    commandHandlers_["NOOP"] = &CanalCommand::handleNoopCommand;
    commandHandlers_["SYST"] = &CanalCommand::handleSystCommand;
    // Transfer parameters
    commandHandlers_["PORT"] = &CanalCommand::handlePortCommand; 
    commandHandlers_["PASV"] = &CanalCommand::handlePasvCommand;
    commandHandlers_["TYPE"] = &CanalCommand::handleTypeCommand; // Todo other type
    // information
    commandHandlers_["SIZE"] = &CanalCommand::handleSizeCommand;
    commandHandlers_["MDTM"] = &CanalCommand::handleMdtmCommand;
    commandHandlers_["STAT"] = &CanalCommand::handleStatCommand;
    commandHandlers_["FEAT"] = &CanalCommand::handleFeatCommand;
    // File action
    commandHandlers_["NLST"] = &CanalCommand::handleNlstCommand;
    commandHandlers_["LIST"] = &CanalCommand::handleListCommand;
    commandHandlers_["PWD"] = &CanalCommand::handlePwdCommand;
    commandHandlers_["CWD"] = &CanalCommand::handleCwdCommand;
    commandHandlers_["CDUP"] = &CanalCommand::handleCdupCommand;
    commandHandlers_["DELE"] = &CanalCommand::handleDeleCommand;
    commandHandlers_["MKD"] = &CanalCommand::handleMkdCommand;
    commandHandlers_["RMD"] = &CanalCommand::handleRmdCommand;
    // Logout
    commandHandlers_["QUIT"] = &CanalCommand::handleQuitCommand;
}

static std::string getLogFileName() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);

    std::ostringstream oss;
    oss << FTP_DEFAULT_DIR_LOG;
    oss << "log_" << (now->tm_year + 1900) << '-'
        << (now->tm_mon + 1) << '-'
        << now->tm_mday << ".log";
    return oss.str();
}

std::string CanalCommand::getTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_time);
    std::ostringstream time_;
    time_ << "["
               << std::setw(2) << std::setfill('0') << local_time->tm_hour << ":" << std::setw(2) << std::setfill('0') << local_time->tm_min << " "
               << std::setw(2) << std::setfill('0') << local_time->tm_mday << "/" << std::setw(2) << std::setfill('0') << (local_time->tm_mon + 1) << "/" << (local_time->tm_year % 100)
               << "] ";
    return time_.str();
}

void CanalCommand::setLogClient(std::string message) {
    std::ostringstream logMessage;
    logMessage << getTime();
    if (client_) {
        try {
            if (client_->username.empty()) {
                logMessage << client_->socket_fd << " (Not Log) : ";
            } else {
                logMessage << client_->socket_fd << " (" << client_->username << ") : ";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error accessing client data: " << e.what() << std::endl;
            logMessage << "undefined (Not Log) : ";
        }
    } else {
        logMessage << "undefined (Not Log) : ";
    }
    logMessage << message;

    // log Consol
    std::cout << logMessage.str() << std::endl;
    std::cout << std::endl;

    // SaveLog
    std::string logFileName = getLogFileName();
    std::ofstream logFile(logFileName, std::ios_base::app);
    if (logFile.is_open()) {
        logFile << logMessage.str() << std::endl;
    } else {
        std::cerr << "Erreur : impossible d'ouvrir le fichier de log." << std::endl;
    }
}

void CanalCommand::setupServer(int port) {
    std::ostringstream logMessage;

    serverSocketCommand_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketCommand_ < 0) {
        serverStart_ = false;
        return;
    }

    serverAddrCommand_.sin_family = AF_INET;
    serverAddrCommand_.sin_addr.s_addr = INADDR_ANY;
    serverAddrCommand_.sin_port = htons(port);

    if (bind(serverSocketCommand_, (struct sockaddr *)&serverAddrCommand_, sizeof(serverAddrCommand_)) < 0) {
        close(serverSocketCommand_);
        serverStart_ = false;
        return;
    }

    if (listen(serverSocketCommand_, 5) < 0) {
        close(serverSocketCommand_);
        serverStart_ = false;
        return;
    }

    serverStart_ = true;
    logMessage << getTime() << " Serveur Vortex FTP Start, Port : " << port << "\n";
    std::cout << logMessage.str() << std::endl;
}

void CanalCommand::sendToClient(FTPClient *client, const std::string &message) {
    if (client->is_secure) {
        if (sslConnect_.sslSend(client->ssl, message.c_str()) <= 0) {
            std::ostringstream logMessage;
            logMessage << "Erreur d'écriture sur le socket SSL " << SSL_get_fd(client->ssl) << ": " << std::strerror(errno) << std::endl;
            setLogClient(logMessage.str());
        }
    } else {
        if (send(client->socket_fd, message.c_str(), message.length(), 0) < 0) {
            std::ostringstream logMessage;
            logMessage << "Erreur d'écriture sur le socket " << client->socket_fd << ": " << std::strerror(errno) << std::endl;
            setLogClient(logMessage.str());
        }
    }
}

bool CanalCommand::handleClient(FTPClient *client) {
    char buffer[configServer_->getConfigValue<int>(configServer_->ConfigKey::BUFFER_SIZE_COMMAND_)];
    ssize_t bytesRead;
    std::ostringstream logMessage;

    if (client->is_secure) {
      bytesRead = SSL_read(client->ssl, buffer, sizeof(buffer) - 1);
    } else {
        bytesRead = read(client->socket_fd, buffer, sizeof(buffer) - 1);
    }

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string command(buffer);
        processCommand(client, command);
        return true;
    } else if (bytesRead == 0) {
        logMessage << "Client Disconnect" << std::endl;
        setLogClient(logMessage.str());
        return false;
    } else {
        if (client->is_secure) {
            int ssl_error = SSL_get_error(client->ssl, bytesRead);
            logMessage << "Erreur de lecture du socket SSL " << client->socket_fd << ": " << ssl_error << " - " << std::strerror(errno) << std::endl;
        } else {
            logMessage << "Erreur de lecture du socket " << client->socket_fd << ": " << std::strerror(errno) << std::endl;
        }
        std::cout << (logMessage.str()) << std::endl;
        return false;
    }
}

int CanalCommand::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    std::ostringstream logMessage;

    int clientSocket = accept(serverSocketCommand_, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        logMessage << "Erreur d'acceptation de la connexion: " << std::strerror(errno) << std::endl;
        std::cout << (logMessage.str()) << std::endl;
        return -1;
    }

    std::string clientIP = inet_ntoa(clientAddr.sin_addr);
    logMessage << "Client connected (IP: " << clientIP << ")" << std::endl;
    setLogClient(logMessage.str());
    std::string message = "220 " + configServer_->getConfigValue<std::string>(configServer_->ConfigKey::FTP_BANNER_) + "\r\n";
    if (send(clientSocket, message.c_str(), message.length(), 0) < 0) {
        std::ostringstream logMessage;
        logMessage << "Erreur d'écriture sur le socket " << clientSocket << ": " << std::strerror(errno) << std::endl;
        setLogClient(logMessage.str());
    }
    return clientSocket;
}

int CanalCommand::getServerSocket() const {
    return serverSocketCommand_;
}

void CanalCommand::processCommand(FTPClient *client, const std::string &command) {
    client_ = client;
    std::vector<std::string> commands;

    std::regex re("\\s+");
    std::sregex_token_iterator it(command.begin(), command.end(), re, -1);
    std::sregex_token_iterator end;
    while (it != end) {
        commands.push_back(*it++);
    }

    std::ostringstream logMessage;
    bool isAuthenticated = client->authenticated;
    bool isCommandAllowed = (isAuthenticated && commands[0] != "USER" && commands[0] != "PASS") ||
                            (!isAuthenticated && (commands[0] == "USER" || commands[0] == "PASS"));

    if (isCommandAllowed || commands[0] == "QUIT" || commands[0] == "AUTH") {
        std::ostringstream logMessage;
        auto it = commandHandlers_.find(commands[0]);
        if (it != commandHandlers_.end()) {
            logMessage << "Command : ";
            for (size_t i = 0; i < commands.size(); i++)
                logMessage << commands[i] << ' ';
            setLogClient(logMessage.str());
            if (commands[0] == "AUTH") {
                (this->*(it->second))(client, commands);
            } else {
                queueClient_->enqueueClientTask(client->socket_fd, [this, client, commands, it]() {
                    (this->*(it->second))(client, commands);
                });
            }
        } else {
            logMessage << "500 Syntax error, command unrecognized. \"" << commands[0] << '"' << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "500 Syntax error, command unrecognized.\r\n");
        }
    } else {
        if (isAuthenticated && (commands[0] == "USER" || commands[0] == "PASS")) {
            logMessage << "530 Bad sequence of commands. Already Logged \"" << command << '"' << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "530 Bad sequence of commands.\r\n");
        } else {
            logMessage << "530 Not logged in or command not allowed. \"" << command << '"' << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "530 Not logged in or command not allowed.\r\n");
        }
    }
}

//Canal command
// AUTH, USER, PASS, REIN, QUIT, PASV, PORT, SIZE, NOOP, SYST, ALLO, STAT, FEAT

void CanalCommand::handleAuthCommand(FTPClient *client, std::vector<std::string> commands) {
    setLogClient(commands[0]);
    if (commands.size() < 2) {
        setLogClient("Syntax error in parameters or arguments.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::string authMethod = commands[1];
    if (client->is_secure) {
        std::cout << "534 Security mechanism already established." << std::endl;
        sendToClient(client, "534 Security mechanism already established.\r\n");
        return;
    }

    SSL* ssl = nullptr;
    if (authMethod == "TLS" || authMethod == "SSL") {
        sendToClient(client, "234 Proceed with negotiation\r\n");
        ssl = sslConnect_.acceptConnection(client->socket_fd);
        if (ssl) {
            client->ssl = ssl;
            client->is_secure = true;
            client->is_tls_negotiated = (authMethod == "TLS");
            client->type_protocole = TypeProtocole::FTPES;
            client->context = sslConnect_.getContext();
            setLogClient("SSL/TLS negotiation successful");
        } else {
            setLogClient("Service not available, closing control connection.");
            sendToClient(client, "421 Service not available, closing control connection.\r\n");
        }
    } else {
        sendToClient(client, "504 Command not implemented for that parameter\r\n");
    }
}

void CanalCommand::handleUserCommand(FTPClient* client, std::vector<std::string> command) {
    if (client->username.empty()) {
        if (command.size() < 2) {
            setLogClient("501 Syntax error in parameters or arguments.");
            sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
            return;
        }

        std::string username = command[1];
        std::ifstream userFile(FTP_DEFAULT_FILE_USERS);
        if (!userFile.is_open()) {
            setLogClient("Erreur: Impossible d'ouvrir le fichier des utilisateurs.");
            sendToClient(client, "550 File not found.\r\n");
            return;
        }

        std::string line;
        bool userFound = false;
        while (std::getline(userFile, line)) {
            std::stringstream ss(line);
            std::string fileUser, filePassword;
            if (std::getline(ss, fileUser, ':') && std::getline(ss, filePassword)) {
                if (fileUser == username) {
                    userFound = true;
                    break;
                }
            }
        }
        userFile.close();

        if (userFound) {
            client->username = username;
            sendToClient(client, "331 User name okay, need password.\r\n");
        } else {
            setLogClient("530 Not logged in.");
            sendToClient(client, "530 Not logged in.\r\n");
        }
    } else {
        setLogClient("503 Bad sequence of commands.");
        sendToClient(client, "503 Bad sequence of commands.\r\n");
    }
}

static std::string hashPassword(const std::string& password, const std::string& salt) {
    std::string saltedPassword = password + salt;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) throw std::runtime_error("Unable to create EVP_MD_CTX");

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) throw std::runtime_error("Unable to initialize digest");
    if (1 != EVP_DigestUpdate(mdctx, saltedPassword.c_str(), saltedPassword.size())) throw std::runtime_error("Unable to update digest");
    if (1 != EVP_DigestFinal_ex(mdctx, hash, &hashLen)) throw std::runtime_error("Unable to finalize digest");

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)hash[i];
    }
    return ss.str();
}

void CanalCommand::handlePassCommand(FTPClient* client, std::vector<std::string> command) {
    if (client->username.empty()) {
        setLogClient("Error: Bad sequence of commands.");
        sendToClient(client, "503 Bad sequence of commands.\r\n");
        return;
    }

    if (command.size() < 2) {
        setLogClient("Error: Syntax error in parameters or arguments.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::string password = command[1];
    std::ifstream userFile(FTP_DEFAULT_FILE_USERS);
    if (!userFile.is_open()) {
        setLogClient("Erreur: Impossible d'ouvrir le fichier des utilisateurs.");
        sendToClient(client, "550 File not found.\r\n");
        return;
    }

    std::string line;
    bool passwordMatched = false;
    while (std::getline(userFile, line)) {
        std::size_t delimiterPos1 = line.find(':');
        if (delimiterPos1 != std::string::npos) {
            std::string fileUser = line.substr(0, delimiterPos1);
            std::size_t delimiterPos2 = line.find(':', delimiterPos1 + 1);
            if (delimiterPos2 != std::string::npos) {
                std::string filePassword = line.substr(delimiterPos1 + 1, delimiterPos2 - delimiterPos1 - 1);
                std::string salt = line.substr(delimiterPos2 + 1);

                if (fileUser == client->username) {
                    std::string hashedPassword = hashPassword(password, salt);
                    if (filePassword == hashedPassword) {
                        passwordMatched = true;
                        break;
                    }
                }
            }
        }
    }
    userFile.close();

    if (passwordMatched) {
        client->authenticated = true;
        sendToClient(client, "230 User logged in, proceed.\r\n");
    } else {
        sendToClient(client, "530 Not logged in.\r\n");
    }
}

void CanalCommand::handleReinCommand(FTPClient* client, std::vector<std::string> command) {
    if (!client) {
        return;
    }

    if (client->is_secure && client->ssl) {
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
        client->ssl = nullptr;
        client->is_tls_negotiated = false;
    }

    client->username.clear();
    client->current_directory = "/";
    client->authenticated = false;

    sendToClient(client, "220 Service ready for new user.\r\n");
}

void CanalCommand::handleQuitCommand(FTPClient* client, std::vector<std::string> command) {
    sendToClient(client, "250 Command okay.\r\n");
}

void CanalCommand::handleTypeCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Erreur: Invalid command format.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    if (command[1] == "I") {
        client->data_info.type = Type::I;
        setLogClient("Setting type to BINARY (I).");
        sendToClient(client, "200 Type set to I.\r\n");
    } else if (command[1] == "A") {
        client->data_info.type = Type::A;
        setLogClient("Setting type to ASCII (A).");
        sendToClient(client, "200 Type set to A.\r\n");
    } else {
        std::ostringstream logMessage;
        logMessage << "Socket: [" << client->socket_fd << "], Invalid type parameter: " << command[1] << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "504 Command not implemented for that parameter.\r\n");
    }
}

void CanalCommand::handlePwdCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() > 1) {
        setLogClient("Error: Too many arguments for PWD command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }
    sendToClient(client, std::string("257 " + client->current_directory + " is the current directory\r\n"));
}

void CanalCommand::handleMkdCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Incorrect number of arguments for MKD command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::filesystem::path complete_path = FTP_DEFAULT_DIR_USER(client->username) + client->current_directory + command[1];
    std::ostringstream logMessage;
    try {
        if (!std::filesystem::exists(complete_path)) {
            std::filesystem::create_directories(complete_path);
            logMessage << "Directory for user " << client->current_directory + command[1] + "/" << " created successfully." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "257 " + client->current_directory + command[1] + "/" + " directory created.\r\n");
        } else {
            logMessage << "Error: Directory for user " << command[1] << " already exists." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 Directory already exists.\r\n");
        }
    } catch (const std::filesystem::filesystem_error& e) {
        logMessage << "Error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unable to create directory.\r\n");
    } catch (const std::exception& e) {
        logMessage << "Unexpected error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

static std::string simplifier(const std::string& chaine) {
    std::regex rgx("/+");
    return std::regex_replace(chaine, rgx, "/");
}

void CanalCommand::handleCwdCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Incorrect number of arguments for CWD command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::string newDirectory = command[1];
    bool is_child_dir = (command[1][0] != '/');
    std::string fullPath = FTP_DEFAULT_DIR_USER(client->username);
    fullPath += (is_child_dir ? client->current_directory : "");
    fullPath += newDirectory;
    fullPath += "/";
    fullPath = simplifier(fullPath);
    std::ostringstream logMessage;
    try {
        if (std::filesystem::exists(fullPath) && std::filesystem::is_directory(fullPath)) {
            if (is_child_dir) {
                client->current_directory += newDirectory;
            } else {
                client->current_directory = newDirectory;
            }
            client->current_directory += "/";
            client->current_directory = simplifier(client->current_directory);
            logMessage << "Directory changed to \"" << client->current_directory << "\" successfully." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "250 Directory successfully changed.\r\n");
        } else {
            logMessage << "Error: Directory \"" << fullPath << "\" does not exist or is not a directory." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 Failed to change directory. Directory does not exist.\r\n");
        }
    } catch (const std::filesystem::filesystem_error& e) {
        logMessage << "Error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unable to change directory due to a server error.\r\n");
    } catch (const std::exception& e) {
        logMessage << "Unexpected error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

void CanalCommand::handleCdupCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 1) {
        setLogClient("Error: Incorrect number of arguments for CDUP command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    if (!client->current_directory.empty() && client->current_directory != "/" && client->current_directory.back() == '/') {
        client->current_directory.pop_back(); 
    } else {
            sendToClient(client, "550 Cannot go up from root directory.\r\n");
            setLogClient("Error: Cannot go up from root directory.");
        return;
    }
    std::filesystem::path currentPath = client->current_directory;
    client->current_directory = currentPath.parent_path();
    client->current_directory += client->current_directory == "/" ? "" : "/";
    client->current_directory = simplifier(client->current_directory);
    std::cout << "Directory changed to \"" << client->current_directory << "\" successfully." << std::endl;
    sendToClient(client, "250 Directory successfully changed.\r\n");
}

void CanalCommand::handleRmdCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Incorrect number of arguments for RMD command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::filesystem::path complete_path = FTP_DEFAULT_DIR_USER(client->username) + client->current_directory + command[1];
    std::ostringstream logMessage;
    try {
        if (!std::filesystem::exists(complete_path)) {
            logMessage << "Error: Directory " << complete_path << " does not exist." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 Directory not found.\r\n");
            return;
        }
        if (!std::filesystem::is_directory(complete_path)) {
            logMessage << "Error: Path " << complete_path << " is not a directory." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 Not a directory.\r\n");
            return;
        }

        if (std::filesystem::remove(complete_path)) {
            logMessage << "Directory " << complete_path << " removed successfully." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "250 Directory successfully removed.\r\n");
        } else {
            logMessage << "Error: Failed to remove directory " << complete_path << "." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 Failed to remove directory.\r\n");
        }
    } catch (const std::filesystem::filesystem_error& e) {
        logMessage << "Error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unable to remove directory.\r\n");
    } catch (const std::exception& e) {
        logMessage << "Unexpected error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

void CanalCommand::handleDeleCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Incorrect number of arguments for DELE command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::filesystem::path complete_path = FTP_DEFAULT_DIR_USER(client->username) + client->current_directory + command[1];
    std::ostringstream logMessage;
    try {
        if (!std::filesystem::exists(complete_path)) {
            logMessage << "Error: File " << complete_path << " does not exist." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 File not found.\r\n");
            return;
        }
        if (!std::filesystem::is_regular_file(complete_path)) {
            logMessage << "Error: Path " << complete_path << " is not a file." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 Not a file.\r\n");
            return;
        }
        if (std::filesystem::remove(complete_path)) {
            logMessage << "File " << complete_path << " removed successfully." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "250 File successfully deleted.\r\n");
        } else {
            logMessage << "Error: Failed to remove file " << complete_path << "." << std::endl;
            setLogClient(logMessage.str());
            sendToClient(client, "550 Failed to delete file.\r\n");
        }
    } catch (const std::filesystem::filesystem_error& e) {
        logMessage << "Error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unable to delete file.\r\n");
    } catch (const std::exception& e) {
        logMessage << "Unexpected error: " << e.what() << std::endl;
        setLogClient(logMessage.str());
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

int CanalCommand::createAvailablePort() {
    std::ostringstream logMessage;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        logMessage << "Erreur lors de la création du socket" << std::endl;
        setLogClient(logMessage.str());
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = 0;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket);
        logMessage << "Erreur lors du bind" << std::endl;
        setLogClient(logMessage.str());
        return -1;
    }

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getsockname(server_socket, (struct sockaddr*)&addr, &addr_len) < 0) {
        close(server_socket);
        logMessage << "Erreur lors de la récupération du nom du socket" << std::endl;
        setLogClient(logMessage.str());
        return -1;
    }

    int port = ntohs(addr.sin_port);
    portUse_.push_back(port);
    close(server_socket);

    return port;
}

void CanalCommand::handlePasvCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() > 1) {
        setLogClient("Error: Syntax error in parameters or arguments.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    int port = createAvailablePort();
    if (port == -1) {
        setLogClient("Error: Can't open data connection. Create Port");
        sendToClient(client, "425 Can't open data connection.\r\n");
        return;
    }

    client->data_info.port_client = port;

    int p1 = port / 256;
    int p2 = port % 256;

    CanalData canalData(&client->data_info, client->context);
    if (!canalData.setupConnection()) {
        setLogClient("Error: Can't open data connection. Create Socket");
        sendToClient(client, "425 Can't open data connection.\r\n");
        return;
    }

    client->data_info.mode = FTPMode::Passive;

    std::ostringstream logMessage;
    logMessage << "Server requested with port: " << port << std::endl;
    setLogClient(logMessage.str());
    sendToClient(client, std::string("227 Entering Passive Mode (" + std::string(IP_SERVER_FORMAT) + "," + std::to_string(p1) + "," + std::to_string(p2) + ").\r\n"));
    client->data_info.server_socket = canalData.serverSocket_;
}

void CanalCommand::handlePortCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Syntax error in parameters or arguments.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::string portCmd = command[1];
    std::replace(portCmd.begin(), portCmd.end(), ',', ' ');
    std::istringstream iss(portCmd);
    int a1, a2, a3, a4, p1, p2;

    if (!(iss >> a1 >> a2 >> a3 >> a4 >> p1 >> p2)) {
        setLogClient("Error: Syntax error in parameters or arguments.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    client->data_info.mode = FTPMode::Active;

    int port = p1 * 256 + p2;
    client->data_info.port_client = port;
    std::ostringstream logMessage;
    logMessage << "Client requested PORT command with port: " << port << std::endl;
    setLogClient(logMessage.str());
    sendToClient(client, "250 Command okay.\r\n");
}

std::string get_last_write_time(const std::filesystem::path& filePath) {
    std::filesystem::file_time_type fileTime = std::filesystem::last_write_time(filePath);
    std::chrono::system_clock::time_point sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
    std::tm* tm = std::gmtime(&cftime);
    std::string timeString = std::asctime(tm);
    timeString.pop_back();

    return timeString;
}

void CanalCommand::handleMdtmCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Incorrect number of arguments for MDTM command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::string filename = command[1];
    std::string filepath = std::string(FTP_DEFAULT_DIR_USER(client->username)) + "/" + filename;
    std::filesystem::path filePath(filepath);

    try {
        sendToClient(client, "213 " + get_last_write_time(filepath) + "\r\n");
    } catch (const std::filesystem::filesystem_error& e) {
        setLogClient("Error: File not found.");
        sendToClient(client, "550 File not found.\r\n");
    }
}

void CanalCommand::handleSizeCommand(FTPClient *client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Incorrect number of arguments for Size command.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::string filename = command[1];
    std::string filepath = std::string(FTP_DEFAULT_DIR_USER(client->username)) + "/" + filename;
    try {
        sendToClient(client, "213 " + std::to_string(std::filesystem::file_size(filepath)) + " octets" + "\r\n");
    } catch (const std::filesystem::filesystem_error& e) {
        sendToClient(client, "550 File not found.\r\n");
        setLogClient("Error: File not found.");

    }
}

void CanalCommand::handleNoopCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 1) {
        sendToClient(client, "NOOP command should have no arguments.\r\n");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    try {
        sendToClient(client, "200 NOOP\r\n");
    } catch (const std::exception& e) {
        setLogClient("Error: Unexpected");
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

void CanalCommand::handleAlloCommand(FTPClient* client, std::vector<std::string> command) {
    try {
        std::cout << "202 Command not implemented, superfluous at this site." << std::endl;
        sendToClient(client, "202 Command not implemented, superfluous at this site.");
    } catch (const std::exception& e) {
        setLogClient("Error: Unexpected");
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

void CanalCommand::handleSystCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 1) {
        setLogClient("Error: SYST command should have no arguments");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }
    try {
        struct utsname system_info;
        if (uname(&system_info) != 0) {
            setLogClient("Error: Unable to retrieve system information.");
            sendToClient(client, "550 Unable to retrieve system information.\r\n");
            return;
        }
        sendToClient(client, std::string("215 ") + system_info.sysname + " TYPE: L8\r\n");
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

static std::string get_permissions_string(std::filesystem::perms p) {
    std::string permission_string;

    permission_string += (p & std::filesystem::perms::owner_read)  != std::filesystem::perms::none ? 'r' : '-';
    permission_string += (p & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? 'w' : '-';
    permission_string += (p & std::filesystem::perms::owner_exec)  != std::filesystem::perms::none ? 'x' : '-';

    permission_string += (p & std::filesystem::perms::group_read)  != std::filesystem::perms::none ? 'r' : '-';
    permission_string += (p & std::filesystem::perms::group_write) != std::filesystem::perms::none ? 'w' : '-';
    permission_string += (p & std::filesystem::perms::group_exec)  != std::filesystem::perms::none ? 'x' : '-';

    permission_string += (p & std::filesystem::perms::others_read)  != std::filesystem::perms::none ? 'r' : '-';
    permission_string += (p & std::filesystem::perms::others_write) != std::filesystem::perms::none ? 'w' : '-';
    permission_string += (p & std::filesystem::perms::others_exec)  != std::filesystem::perms::none ? 'x' : '-';

    return permission_string;
}

void CanalCommand::handleStatCommand(FTPClient* client, std::vector<std::string> command) {
    try {
        if (command.size() == 1) {
            setLogClient("Error: Command not implemented, superfluous at this site.");
            sendToClient(client, "202 Command not implemented, superfluous at this site.\r\n");
        } else {
            std::string filename = command[1];
            std::string filepath = std::string(FTP_DEFAULT_DIR_USER(client->username)) + "/" + filename;
            if (std::filesystem::is_regular_file(filepath)) {
                sendToClient(client, std::string("213-Status of ") + std::string(filename) + "\n"
                                                    + std::string("Size: ") + std::to_string(std::filesystem::file_size(filepath)) + " octets\n"
                                                    + std::string("Permissions: ") + std::string(get_permissions_string(std::filesystem::status(filepath).permissions())) + "\n"
                                                    + std::string("Last modified: ") + get_last_write_time(filepath) + "\n" + std::string("213 End of file status.\r\n")
                             );
            } else {
                sendToClient(client, "202 Command not implemented, superfluous at this site.\r\n");
            }
        }

    } catch (const std::exception& e) {
        setLogClient("Error: Unexpected.");
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}

void CanalCommand::handleFeatCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 1) {
        setLogClient("Error: FEAT command should have no arguments");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    std::string response = "211-Features:\r\n";
    for (const auto& handler : commandHandlers_) {
        response += " " + handler.first + "\r\n";
    }
    response += "211 End\r\n";
    sendToClient(client, response);
}

//Canal data
// NLST, STOR, RETR, LIST
void CanalCommand::handleNlstCommand(FTPClient* client, std::vector<std::string> command) { //todo (passif mode)
    if (client->data_info.mode == FTPMode::Undefined) {
        setLogClient("Error: Can't open data connection. FTPMode Undefined");
        sendToClient(client, "425 Can't open data connection. FTPMode Undefined\r\n");
        return;
    }

    queueClient_->enqueueClientTask(client->socket_fd, [this, command, client]() {

        std::filesystem::path currentPath = FTP_DEFAULT_DIR_USER(client->username) + client->current_directory + command[1];
        if (!std::filesystem::exists(currentPath) || !std::filesystem::is_directory(currentPath)) {
            sendToClient(client, "550 Failed to list directory: Directory does not exist.\r\n");
            setLogClient("Failed to list directory: Directory does not exist.");
            return;
        }

        std::stringstream fileList;
        for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
            fileList << entry.path().filename().string() << "\r\n";
        }

        CanalData canalData(&client->data_info, client->context);
        if (!canalData.setupConnection()) {
            setLogClient("Error: Can't open data connection.");
            sendToClient(client, "425 Can't open data connection.\r\n");
            return;
        }
        sendToClient(client, "150 Here comes the directory listing.\r\n");
        if (client->data_info.mode == FTPMode::Passive) {
            if (!canalData.acceptConnection()) {
                setLogClient("Error: Can't open data connection.");
                sendToClient(client, "425 Can't open data connection.\r\n");
                return;
            }
        }

        if (fileList.str().empty()) {
            sendToClient(client, "226 No files found.\r\n");
            setLogClient("No files found.");
        } else {
            if (canalData.sendData(fileList.str().c_str(), fileList.str().size())) {
                sendToClient(client, "226 Directory send OK.\r\n");
            } else {
                sendToClient(client, "426 Connection closed; transfer aborted.\r\n");
                setLogClient("Connection closed; transfer aborted..");
            }
        }
        canalData.closeConnection();
    });
}

static std::string getFilePermissions(const struct stat& fileStat) {
    std::string perms;

    if (S_ISDIR(fileStat.st_mode)) {
        perms += 'd';
    } else if (S_ISREG(fileStat.st_mode)) {
        perms += '-';
    } else if (S_ISLNK(fileStat.st_mode)) {
        perms += 'l';
    } else if (S_ISCHR(fileStat.st_mode)) {
        perms += 'c';
    } else if (S_ISBLK(fileStat.st_mode)) {
        perms += 'b';
    } else if (S_ISFIFO(fileStat.st_mode)) {
        perms += 'p';  // FIFO or pipe
    } else if (S_ISSOCK(fileStat.st_mode)) {
        perms += 's';
    } else {
        perms += '?';
    }
    perms += (fileStat.st_mode & S_IRUSR) ? 'r' : '-';
    perms += (fileStat.st_mode & S_IWUSR) ? 'w' : '-';
    perms += (fileStat.st_mode & S_IXUSR) ? 'x' : '-';
    perms += (fileStat.st_mode & S_IRGRP) ? 'r' : '-';
    perms += (fileStat.st_mode & S_IWGRP) ? 'w' : '-';
    perms += (fileStat.st_mode & S_IXGRP) ? 'x' : '-';
    perms += (fileStat.st_mode & S_IROTH) ? 'r' : '-';
    perms += (fileStat.st_mode & S_IWOTH) ? 'w' : '-';
    perms += (fileStat.st_mode & S_IXOTH) ? 'x' : '-';
    return perms;
}

static std::string getFileOwnerName(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}

static std::string getFileGroupName(gid_t gid) {
    struct group *gr = getgrgid(gid);
    return gr ? gr->gr_name : "unknown";
}

static std::string formatFileSize(off_t size) {
    std::stringstream ss;
    ss << std::setw(10) << std::right << size;
    return ss.str();
}

static std::string formatFileDate(const struct stat& fileStat) {
    std::tm* ptm = std::localtime(&fileStat.st_mtime);
    std::stringstream ss;
    ss << std::put_time(ptm, "%b %d %H:%M");
    return ss.str();
}

void CanalCommand::handleListCommand(FTPClient* client, std::vector<std::string> command) {
    if (client->data_info.mode == FTPMode::Undefined) {
        std::cout << "Undefined >>>" << std::endl;
    }
    if (client->data_info.mode == FTPMode::Active) {
        std::cout << "Active    >>>" << std::endl;
    }
    if (client->data_info.mode == FTPMode::Passive) {
        std::cout << "Passif    >>>" << std::endl;
    }

    if (client->data_info.mode == FTPMode::Undefined) {
        setLogClient("Error: Can't open data connection. FTPMode Undefined");
        sendToClient(client, "425 Can't open data connection. FTPMode Undefined\r\n");
        return;
    }

    queueClient_->enqueueClientTask(client->socket_fd, [this, command, client]() {
        std::string currentPath = FTP_DEFAULT_DIR_USER(client->username) + client->current_directory + (command.size() > 2 ? command[1] : "");
        struct stat pathStat;
        if (stat(currentPath.c_str(), &pathStat) != 0 || !S_ISDIR(pathStat.st_mode)) {
            setLogClient("Failed to list directory: Directory does not exist.");
            sendToClient(client, "550 Failed to list directory: Directory does not exist.\r\n");
            return;
        }

        DIR* dir = opendir(currentPath.c_str());
        if (!dir) {
            setLogClient("Failed to open directory.");
            sendToClient(client, "550 Failed to open directory.\r\n");
            return;
        }

        struct dirent* entry;
        std::stringstream fileList;
        while ((entry = readdir(dir)) != nullptr) {
            std::string entryPath = currentPath + "/" + entry->d_name;
            struct stat entryStat;

            if (stat(entryPath.c_str(), &entryStat) == 0) {
                std::string permissions = getFilePermissions(entryStat);
                std::string owner = getFileOwnerName(entryStat.st_uid);
                std::string group = getFileGroupName(entryStat.st_gid);
                std::string size = formatFileSize(entryStat.st_size);
                std::string date = formatFileDate(entryStat);
                std::string filename = entry->d_name;

                fileList << permissions << " "
                         << std::setw(2) << std::right << "1" << " "
                         << owner << " "
                         << group << " "
                         << size << " "
                         << date << " "
                         << filename << "\r\n";
            }
        }
        closedir(dir);

        CanalData canalData(&client->data_info, client->context);
        if (!canalData.setupConnection()) {
            setLogClient("Error: Can't open data connection.");
            sendToClient(client, "425 Can't open data connection.\r\n");
            return;
        }
        sendToClient(client, "150 Here comes the directory listing.\r\n");
        if (client->data_info.mode == FTPMode::Passive) {
            if (!canalData.acceptConnection()) {
                setLogClient("Error: Can't open data connection.");
                sendToClient(client, "425 Can't open data connection.\r\n");
                return;
            }
        }

        std::string fileListStr = fileList.str();
        if (fileListStr.empty()) {
            setLogClient("No files found.");
            sendToClient(client, "226 No files found.\r\n");
        } else {
            if (canalData.sendData(fileListStr.c_str(), fileListStr.size())) {
                sendToClient(client, "226 Directory send OK.\r\n");
            } else {
                setLogClient("Connection closed; transfer aborted.");
                sendToClient(client, "426 Connection closed; transfer aborted.\r\n");
            }
        }
        canalData.closeConnection();
    });
}

void CanalCommand::handleStorCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: Syntax error in parameters or arguments.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    if (client->data_info.mode == FTPMode::Undefined) {
        setLogClient("Error: Can't open data connection. FTPMode Undefined");
        sendToClient(client, "425 Can't open data connection. FTPMode Undefined\r\n");
        return;
    }

    queueClient_->enqueueClientTask(client->socket_fd, [this, client, command]() {

        std::filesystem::path filepath = FTP_DEFAULT_DIR_USER(client->username) + client->current_directory + command[1];
        std::ofstream outFile(filepath, std::ios::out | std::ios::binary);
        if (!outFile) {
            setLogClient("Error: Unable to create the file.");
            sendToClient(client, "451 Requested action aborted: local error in processing.\r\n");
            return;
        }

        CanalData canalData(&client->data_info, client->context);
        if (!canalData.setupConnection()) {
            setLogClient("Error: Can't open data connection.");
            sendToClient(client, "425 Can't open data connection.\r\n");
            return;
        }
        sendToClient(client, "150 Here comes the directory listing.\r\n");
        if (client->data_info.mode == FTPMode::Passive) {
            if (!canalData.acceptConnection()) {
                setLogClient("Error: Can't open data connection.");
                sendToClient(client, "425 Can't open data connection.\r\n");
                return;
            }
        }

        const size_t bufferSize = configServer_->getConfigValue<int>(configServer_->ConfigKey::BUFFER_SIZE_DATA_);
        char buffer[bufferSize];
        ssize_t bytesRead;
        while ((bytesRead = canalData.receiveData(buffer, bufferSize)) > 0) {
            outFile.write(buffer, bytesRead);
        }

        if (bytesRead < 0) {
            setLogClient("Error: Connection closed; transfer aborted.");
            sendToClient(client, "426 Connection closed; transfer aborted.\r\n");
        } else {
            sendToClient(client, "226 Closing data connection; requested file action successful.\r\n");
        }
        outFile.close();
        canalData.closeConnection();
    });
}

void CanalCommand::handleRetrCommand(FTPClient* client, std::vector<std::string> command) { //todo
    if (command.size() != 2) {
        setLogClient("Error: Syntax error in parameters or arguments.");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    if (client->data_info.mode == FTPMode::Undefined) {
        setLogClient("Error: Can't open data connection. FTPMode Undefined");
        sendToClient(client, "425 Can't open data connection. FTPMode Undefined\r\n");
        return;
    }

    queueClient_->enqueueClientTask(client->socket_fd, [this, client, command]() {

        std::filesystem::path filepath = FTP_DEFAULT_DIR_USER(client->username) + client->current_directory + command[1];
        FILE* file = fopen(filepath.c_str(), "rb");
        if (file == nullptr) {
            std::ostringstream logMessage;
            logMessage << "Error: File not found: " << filepath << std::endl;
            setLogClient("Error: Connection closed; transfer aborted..");
            sendToClient(client, "550 File not found.\r\n");
            return;
        }

        CanalData canalData(&client->data_info, client->context);
        if (!canalData.setupConnection()) {
            setLogClient("Error: Can't open data connection.");
            sendToClient(client, "425 Can't open data connection.\r\n");
            return;
        }
        sendToClient(client, "150 Here comes the directory listing.\r\n");
        if (client->data_info.mode == FTPMode::Passive) {
            if (!canalData.acceptConnection()) {
                setLogClient("Error: Can't open data connection.");
                sendToClient(client, "425 Can't open data connection.\r\n");
                return;
            }
        }

        const size_t bufferSize = configServer_->getConfigValue<int>(configServer_->ConfigKey::BUFFER_SIZE_DATA_);
        char buffer[configServer_->getConfigValue<int>(configServer_->ConfigKey::BUFFER_SIZE_DATA_)];
        ssize_t bytesRead;
        while ((bytesRead = fread(buffer, 1, bufferSize, file)) > 0) {
            canalData.sendData(buffer, bytesRead);
        }
        fclose(file);

        sendToClient(client, "226 Closing data connection; requested file action successful.\r\n");
        canalData.closeConnection();
    });
}

// FTPS, FTPES
void CanalCommand::handlePbszCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: PBSZ command should have one argument");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    try {
        int size = std::stoi(command[1]);
        if (size < 0) {
            setLogClient("Error: Invalid buffer size.");
            sendToClient(client, "501 Invalid buffer size.\r\n");
            return;
        }
        sendToClient(client, "200 Command OK.\r\n");
    } catch (const std::invalid_argument& e) {
        setLogClient("Error: Invalid buffer size.");
        sendToClient(client, "501 Invalid buffer size.\r\n");
    }
}

void CanalCommand::handleProtCommand(FTPClient* client, std::vector<std::string> command) {
    if (command.size() != 2) {
        setLogClient("Error: PBSZ command should have one argumen");
        sendToClient(client, "501 Syntax error in parameters or arguments.\r\n");
        return;
    }

    try {
        if (command[1] == "C") {
            client->data_info.protection_level = DataProtectionLevel::None;
            sendToClient(client, "200 Command OK, switch to non-quantified data.\r\n");
            client->data_info.ssl_activate = false;
        } else if(command[1] == "P") {
            client->data_info.protection_level = DataProtectionLevel::Confidential;
            sendToClient(client, "200 Command OK, switch to full data encryption.\r\n");
            client->data_info.ssl_activate = true;
        } else if (command[1] == "S") {
            client->data_info.protection_level = DataProtectionLevel::Security;
            sendToClient(client, "200 Command OK, switch to integrity protection without encryption.\r\n");
            client->data_info.ssl_activate = true;
        } else if (command[1] == "E") {
            client->data_info.protection_level = DataProtectionLevel::Encryption;
            sendToClient(client, "200 Command OK, switch encryption without authentication.\r\n");
            client->data_info.ssl_activate = true;
        } else {
            setLogClient("Error: Command not implemented for that parameter.");
            sendToClient(client, "504 Command not implemented for that parameter.\r\n");
            client->data_info.ssl_activate = false;
        }
    } catch (const std::invalid_argument& e) {
        setLogClient("Error: Unexpected error occurred.");
        sendToClient(client, "550 Unexpected error occurred.\r\n");
    }
}
