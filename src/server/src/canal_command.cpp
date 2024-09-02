#include "../include/canal_command.hpp"
#include "../include/canal_data.hpp"
#include "../include/define.hpp"

CanalCommand::CanalCommand(int port, ClientQueueThreadPool *queueClient)
    : queueClient_(queueClient) {
    setupServer(port);

    // Login
    commandHandlers_["USER"] = &CanalCommand::handleUserCommand;
    commandHandlers_["PASS"] = &CanalCommand::handlePassCommand;
    // Transfer
    commandHandlers_["STOR"] = &CanalCommand::handleStorCommand;
    commandHandlers_["RETR"] = &CanalCommand::handleRetrCommand;
    // Transfer parameters
    commandHandlers_["PORT"] = &CanalCommand::handlePortCommand;
    commandHandlers_["PASV"] = &CanalCommand::handlePasvCommand;
    // File action
    commandHandlers_["NLST"] = &CanalCommand::handleNlstCommand;
    commandHandlers_["PWD"] = &CanalCommand::handlePwdCommand;
    commandHandlers_["CWD"] = &CanalCommand::handleCwdCommand;
    //commandHandlers_["DELE"] = &CanalCommand::handleDeleCommand;
    commandHandlers_["MKD"] = &CanalCommand::handleMkdCommand;
    //commandHandlers_["RMD"] = &CanalCommand::handleRmdCommand;

    // Logout
    commandHandlers_["QUIT"] = &CanalCommand::handleQuitCommand;
}

void CanalCommand::setupServer(int port) {

    serverSocket_command = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_command < 0) {
        std::cerr << "Erreur de création du socket: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    serverAddr_command.sin_family = AF_INET;
    serverAddr_command.sin_addr.s_addr = INADDR_ANY;
    serverAddr_command.sin_port = htons(port);

    if (bind(serverSocket_command, (struct sockaddr *)&serverAddr_command, sizeof(serverAddr_command)) < 0) {
        std::cerr << "Erreur de liaison du socket: " << std::strerror(errno) << std::endl;
        close(serverSocket_command);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket_command, 5) < 0) {
        std::cerr << "Erreur d'écoute du socket: " << std::strerror(errno) << std::endl;
        close(serverSocket_command);
        exit(EXIT_FAILURE);
    }

    std::cout << "Serveur FTP, Port : " << port << std::endl;
}

void CanalCommand::sendToClient(int clientSocket, const std::string &message) {
    ssize_t bytesSent = write(clientSocket, message.c_str(), message.size());
    if (bytesSent < 0) {
        std::cerr << "Erreur d'écriture sur le socket " << clientSocket << ": " << std::strerror(errno) << std::endl;
    }
}

bool CanalCommand::handleClient(FTPClient *client) {
    char buffer[BUFFER_SIZE_COMMAND];
    ssize_t bytesRead = read(client->socket_fd, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string command(buffer);
        processCommand(client, command);
        return true;
    } else if (bytesRead == 0) {
        std::cout << "Client déconnecté proprement, socket " << client->socket_fd << std::endl;
        return false;
    } else {
        std::cerr << "Erreur de lecture du socket " << client->socket_fd << ": " << std::strerror(errno) << std::endl;
        return false;
    }
}

int CanalCommand::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket_command, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket < 0) {
        std::cerr << "Erreur d'acceptation de la connexion: " << std::strerror(errno) << std::endl;
        return -1;
    }

    std::string clientIP = inet_ntoa(clientAddr.sin_addr);
    std::cout << "Client connecté (IP: " << clientIP << ", socket: " << clientSocket << ")" << std::endl;
    return clientSocket;
}

int CanalCommand::getServerSocket() const {
    return serverSocket_command;
}

void CanalCommand::processCommand(FTPClient *client, const std::string &command) {
    this->client = client;
    std::vector<std::string> commands;
    
    std::regex re("\\s+");
    std::sregex_token_iterator it(command.begin(), command.end(), re, -1);
    std::sregex_token_iterator end;
    while (it != end) {
        commands.push_back(*it++);
    }

    bool isAuthenticated = client->authenticated;
    bool isCommandAllowed = (isAuthenticated && commands[0] != "USER" && commands[0] != "PASS") || (!isAuthenticated && (commands[0] == "USER" || commands[0] == "PASS"));
    if (isCommandAllowed) {
        queueClient_->enqueueClientTask(client->socket_fd, [this, client, commands, command]() {    
            auto it = commandHandlers_.find(commands[0]);
            if (it != commandHandlers_.end()) {
                (this->*(it->second))(client, commands);
            } else {
                std::cout << "500 Syntax error, command unrecognized. \"" << commands[0] << '"' << std::endl;
                sendToClient(client->socket_fd, "500 Syntax error, command unrecognized.");
            }
        });
    } else {
        if (isAuthenticated && (commands[0] == "USER" || commands[0] == "PASS")) {
            std::cout << "530 Bad sequence of commands. Already Logged \"" << command << '"' << std::endl;
            sendToClient(client->socket_fd, "530 Bad sequence of commands.");
        } else {
            std::cout << "530 Not logged in or command not allowed. \"" << command << '"' << std::endl;
            sendToClient(client->socket_fd, "530 Not logged in or command not allowed.");
        }
    }
}

//Canal command
// USER, PASS, QUIT, PASV, PORT
void CanalCommand::handleUserCommand(FTPClient* client, std::vector<std::string> command) {
    if (client->username.empty()) {
        if (command.size() < 2) {
            sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
            return;
        }

        std::string username = command[1];
        std::ifstream userFile(FTP_FILE_USERS);
        if (!userFile.is_open()) {
            std::cerr << "Erreur: Impossible d'ouvrir le fichier des utilisateurs." << std::endl;
            sendToClient(client->socket_fd, "550 File not found.");
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
            sendToClient(client->socket_fd, "331 User name okay, need password.");
        } else {
            sendToClient(client->socket_fd, "530 Not logged in.");
        }
    } else {
        sendToClient(client->socket_fd, "503 Bad sequence of commands.");
    }
}

void CanalCommand::handlePassCommand(FTPClient* client, std::vector<std::string> command) {
    if (client->username.empty()) {
        std::cerr << "Error: Bad sequence of commands." << std::endl;
        sendToClient(client->socket_fd, "503 Bad sequence of commands.");
        return;
    }

    if (command.size() < 2) {
        std::cerr << "Error: Syntax error in parameters or arguments." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }

    std::string password = command[1];
    std::ifstream userFile(FTP_FILE_USERS);
    if (!userFile.is_open()) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier des utilisateurs." << std::endl;
        sendToClient(client->socket_fd, "550 File not found.");
        return;
    }

    std::string line;
    bool passwordMatched = false;
    while (std::getline(userFile, line)) {
        std::size_t delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos) {
            std::string fileUser = line.substr(0, delimiterPos);
            std::string filePassword = line.substr(delimiterPos + 1);
            if (fileUser == client->username && filePassword == password) {
                passwordMatched = true;
                break;
            }
        }
    }
    userFile.close();

    if (passwordMatched) {
        client->authenticated = true;
        sendToClient(client->socket_fd, "230 User logged in, proceed.");
    } else {
        sendToClient(client->socket_fd, "530 Not logged in.");
    }
}

void CanalCommand::handleQuitCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: QUIT " << std::endl;
    sendToClient(client->socket_fd, "250 Command okay.");
}

void CanalCommand::handlePwdCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: PWD " << std::endl;
    if (command.size() > 1) {
        std::cerr << "Error: Too many arguments for PWD command." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }
    sendToClient(client->socket_fd, std::string("257 " + client->current_directory + " is the current directory"));
}

void CanalCommand::handleMkdCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: MKD " << std::endl;

    if (command.size() != 2) {
        std::cerr << "Error: Incorrect number of arguments for MKD command." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }

    std::filesystem::path complete_path = FTP_DIR_USER(client->username) + client->current_directory + command[1];
    try {
        if (!std::filesystem::exists(complete_path)) {
            std::filesystem::create_directories(complete_path);
            std::cout << "Directory for user " << client->current_directory + command[1] + "/" << " created successfully." << std::endl;
            sendToClient(client->socket_fd, "257 " + client->current_directory + command[1] + "/" + " directory created.");
        } else {
            std::cerr << "Error: Directory for user " << command[1] << " already exists." << std::endl;
            sendToClient(client->socket_fd, "550 Directory already exists.");
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        sendToClient(client->socket_fd, "550 Unable to create directory.");
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        sendToClient(client->socket_fd, "550 Unexpected error occurred.");
    }
}

void CanalCommand::handleCwdCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: CWD " << std::endl;

    if (command.size() != 2) {
        std::cerr << "Error: Incorrect number of arguments for CWD command." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }

    std::string newDirectory = command[1];
    if (newDirectory == "..") {
        if (!client->current_directory.empty() && client->current_directory != "/" && client->current_directory.back() == '/') {
            client->current_directory.pop_back(); 
        } else {
             sendToClient(client->socket_fd, "550 Cannot go up from root directory.");
            return;
        }
        std::filesystem::path currentPath = client->current_directory;
        client->current_directory = currentPath.parent_path();
        client->current_directory += client->current_directory == "/" ? "" : "/";
        std::cout << "Directory changed to \"" << client->current_directory << "\" successfully." << std::endl;
        sendToClient(client->socket_fd, "250 Directory successfully changed.");
    } else {
        std::string fullPath = FTP_DIR_USER(client->username) + client->current_directory + newDirectory + "/";
        try {
            if (std::filesystem::exists(fullPath) && std::filesystem::is_directory(fullPath)) {
                client->current_directory += newDirectory + "/";
                std::cout << "Directory changed to \"" << client->current_directory << "\" successfully." << std::endl;
                sendToClient(client->socket_fd, "250 Directory successfully changed.");
            } else {
                std::cerr << "Error: Directory \"" << fullPath << "\" does not exist or is not a directory." << std::endl;
                sendToClient(client->socket_fd, "550 Failed to change directory. Directory does not exist.");
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            sendToClient(client->socket_fd, "550 Unable to change directory due to a server error.");
        } catch (const std::exception& e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;
            sendToClient(client->socket_fd, "550 Unexpected error occurred.");
        }
    }
}


int CanalCommand::createAvailablePort() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Erreur lors de la création du socket" << std::endl;
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = 0;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket);
        std::cerr << "Erreur lors du bind" << std::endl;
        return -1;
    }

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getsockname(server_socket, (struct sockaddr*)&addr, &addr_len) < 0) {
        close(server_socket);
        std::cerr << "Erreur lors de la récupération du nom du socket" << std::endl;
        return -1;
    }

    int port = ntohs(addr.sin_port);
    portUse_.push_back(port);
    close(server_socket);

    return port;
}

void CanalCommand::handlePasvCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: PASV " << std::endl;

    if (command.size() > 1) {
        std::cerr << "Error: Syntax error in parameters or arguments." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }

    int port = createAvailablePort();
    if (port == -1) {
        std::cerr << "Error: Can't open data connection." << std::endl;
        sendToClient(client->socket_fd, "425 Can't open data connection.");
        return;
    }

    client->data_info->mode = FTPMode::Passive;
    client->data_info->port_client = port;

    int p1 = port / 256;
    int p2 = port % 256;
    std::cout << "Server requested with port: " << port << std::endl;
    sendToClient(client->socket_fd, std::string("227 Entering Passive Mode (" + std::string(IP_SERVER_FORMAT) + "," + std::to_string(p1) + "," + std::to_string(p2) + ")."));
}

void CanalCommand::handlePortCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: PORT " << std::endl;

    if (command.size() != 2) {
        std::cerr << "Error: Syntax error in parameters or arguments." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }

    std::string portCmd = command[1];
    std::replace(portCmd.begin(), portCmd.end(), ',', ' ');
    std::istringstream iss(portCmd);
    int a1, a2, a3, a4, p1, p2;

    if (!(iss >> a1 >> a2 >> a3 >> a4 >> p1 >> p2)) {
        std::cerr << "Error: Syntax error in parameters or arguments." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }

    client->data_info->mode = FTPMode::Active;

    int port = p1 * 256 + p2;
    client->data_info->port_client = port;

    std::cout << "Client requested PORT command with port: " << port << std::endl;
    sendToClient(client->socket_fd, "250 Command okay.");
}

//Canal data
// NLST, STOR, RETR
void CanalCommand::handleNlstCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: NLST " << std::endl;
    sendToClient(client->socket_fd, "250 Command okay.");
}

void CanalCommand::handleStorCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: STOR " << command[1] << std::endl;

    if (command.size() != 2) {
        std::cerr << "Error: Syntax error in parameters or arguments." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }
    sendToClient(client->socket_fd, "150 File status okay; about to open data connection.");

    queueClient_->enqueueClientTask(client->socket_fd, [this, client, command]() {
        if (!client->data_info || client->data_info->mode == FTPMode::Undefined) {
            std::cerr << "Error: Can't open data connection." << std::endl;
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        CanalData canalData(client->data_info);
        if (!canalData.setupConnection()) {
            std::cerr << "Error: Can't open data connection." << std::endl;
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        std::filesystem::path filepath = FTP_DIR_USER(client->username) + client->current_directory + command[1];
        std::ofstream outFile(filepath, std::ios::out | std::ios::binary);
        if (!outFile) {
            std::cerr << "Error: Unable to create the file." << std::endl;
            sendToClient(client->socket_fd, "451 Requested action aborted: local error in processing.");
            return;
        }

        const size_t bufferSize = BUFFER_SIZE_DATA;
        char buffer[bufferSize];
        ssize_t bytesRead;
        while ((bytesRead = canalData.receiveData(buffer, bufferSize)) > 0) {
            outFile.write(buffer, bytesRead);
        }

        if (bytesRead < 0) {
            std::cerr << "Error: Connection closed; transfer aborted." << std::endl;
            sendToClient(client->socket_fd, "426 Connection closed; transfer aborted.");
        } else {
            sendToClient(client->socket_fd, "226 Closing data connection; requested file action successful.");
        }
        outFile.close();
    });
}

void CanalCommand::handleRetrCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: RETR " << command[1] << std::endl;

    if (command.size() != 2) {
        std::cerr << "Error: Syntax error in parameters or arguments." << std::endl;
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }
    sendToClient(client->socket_fd, "150 File status okay; about to open data connection.");

    queueClient_->enqueueClientTask(client->socket_fd, [this, client, command]() {
        if (!client->data_info || client->data_info->mode == FTPMode::Undefined) {
            std::cerr << "Error: Can't open data connection." << std::endl;
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        CanalData canalData(client->data_info);
        if (!canalData.setupConnection()) {
            std::cerr << "Error: Can't open data connection." << std::endl;
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        std::filesystem::path filepath = FTP_DIR_USER(client->username) + client->current_directory + command[1];
        FILE* file = fopen(filepath.c_str(), "rb");
        if (file == nullptr) {
            std::cerr << "Error: File not found: " << filepath << std::endl;
            sendToClient(client->socket_fd, "550 File not found.");
            return;
        }

        const size_t bufferSize = BUFFER_SIZE_DATA;
        char buffer[bufferSize];
        ssize_t bytesRead;
        while ((bytesRead = fread(buffer, 1, bufferSize, file)) > 0) {
            canalData.sendData(buffer, bytesRead);
        }
        fclose(file);

        sendToClient(client->socket_fd, "226 Closing data connection; requested file action successful.");
    });
}
