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
    commandHandlers_["LIST"] = &CanalCommand::handleListCommand;
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
    char buffer[1024];
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
        queueClient_->enqueueClientTask(client->socket_fd, [this, client, commands, command]()
                                        {    
            auto it = commandHandlers_.find(commands[0]);
            if (it != commandHandlers_.end()) {
                (this->*(it->second))(client, commands);
            } else {
                std::cout << "500 Syntax error, command unrecognized. \"" << commands[0] << '"' << std::endl;
                sendToClient(client->socket_fd, "500 Syntax error, command unrecognized.");
            } });
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

// Login, Logout
void CanalCommand::handleUserCommand(FTPClient* client, std::vector<std::string> command) {
    if (client->username.empty()) {
        std::cout << "\nSocket: [" << client->socket_fd << "], Command: USER" << std::endl;
        client->username = "test";
        sendToClient(client->socket_fd, "331 User name okay, need password.");
    } else {
        sendToClient(client->socket_fd, "503 Bad sequence of commands.");
    }
}

void CanalCommand::handlePassCommand(FTPClient* client, std::vector<std::string> command) {

    if (client->username.empty()) {
        sendToClient(client->socket_fd, "503 Bad sequence of commands.");
        return;
    }
    if (true) { // gestion password et correct
        std::cout << "\nSocket: [" << client->socket_fd << "], Command: PASS" << std::endl;
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

    int port = createAvailablePort();
    if (port == -1) {
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
        sendToClient(client->socket_fd, "501 Syntax error in parameters or arguments.");
        return;
    }

    std::string portCmd = command[1];
    std::replace(portCmd.begin(), portCmd.end(), ',', ' ');
    std::istringstream iss(portCmd);
    int a1, a2, a3, a4, p1, p2;

    if (!(iss >> a1 >> a2 >> a3 >> a4 >> p1 >> p2)) {
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

// File action
void CanalCommand::handleListCommand(FTPClient* client, std::vector<std::string> command) {
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: LIST " << std::endl;
    sendToClient(client->socket_fd, "250 Command okay.");
}

void CanalCommand::handleStorCommand(FTPClient* client, std::vector<std::string> command) { // clien -> server upload
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: STOR " << command[1] << std::endl;
    sendToClient(client->socket_fd, "150 File status okay; about to open data connection.");

    queueClient_->enqueueClientTask(client->socket_fd, [this, client, command]() {
        if (!client->data_info || client->data_info->mode == FTPMode::Undefined) {
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        CanalData canalData(client->data_info);

        if (!canalData.setupConnection()) {
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        std::string base_directory = "/Ftp_Client/" + client->username + "/" + client->current_directory;
        std::string filepath = base_directory + "/" + command[1];
        FILE* file = fopen(filepath.c_str(), "w");
        if (file == nullptr) {
            sendToClient(client->socket_fd, "451 Requested action aborted: local error in processing.");
            return;
        }

        const size_t bufferSize = 4096;
        char buffer[bufferSize];
        ssize_t bytesRead;
        while ((bytesRead = canalData.receiveData(buffer, bufferSize)) > 0) {
            fwrite(buffer, 1, bytesRead, file);
        }

        if (bytesRead < 0) {
            sendToClient(client->socket_fd, "426 Connection closed; transfer aborted.");
        } else {
            sendToClient(client->socket_fd, "226 Closing data connection; requested file action successful.");
        }
        fclose(file);
    });
}

void CanalCommand::handleRetrCommand(FTPClient* client, std::vector<std::string> command) { // server -> client download
    std::cout << "\nSocket: [" << client->socket_fd << "], Command: RETR " << command[1] << std::endl;
    sendToClient(client->socket_fd, "150 File status okay; about to open data connection.");

    queueClient_->enqueueClientTask(client->socket_fd, [this, client, command]() {
        if (!client->data_info || client->data_info->mode == FTPMode::Undefined) {
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        CanalData canalData(client->data_info);

        if (!canalData.setupConnection()) {
            sendToClient(client->socket_fd, "425 Can't open data connection.");
            return;
        }

        std::string base_directory = "/Ftp_Client/" + client->username + "/" + client->current_directory;
        std::string filepath = base_directory + "/" + command[1];
        FILE* file = fopen(filepath.c_str(), "r");
        if (file == nullptr) {
            sendToClient(client->socket_fd, "550 File not found.");
            return;
        }

        const size_t bufferSize = 4096;
        char buffer[bufferSize];
        ssize_t bytesRead;
        while ((bytesRead = fread(buffer, 1, bufferSize, file)) > 0) {
            canalData.sendData(buffer, bytesRead);
        }

        fclose(file);

        sendToClient(client->socket_fd, "226 Closing data connection; requested file action successful.");
    });
}
