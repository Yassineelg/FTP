#pragma once

#include "canal_command.hpp"
#include "poller.hpp"

#include <map>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <cstring>

struct FTPClient {
    int socket_fd;                  // Descripteur de socket du client
    std::string username;           // Nom d'utilisateur pour la session FTP
    std::string current_directory;  // Répertoire de travail actuel du client
    bool authenticated;             // Indique si le client est authentifié
    bool is_uploading;              // Indique si un transfert de fichier est en cours
    std::vector<std::string> transfer_file_paths; // Liste des fichiers en cours de transfert

    FTPClient(int fd = -1)
        : socket_fd(fd), 
          username(""),
          current_directory("/"),
          authenticated(false),
          is_uploading(false) {}

    void reset() {
        username.clear();
        current_directory = "/";
        authenticated = false;
        is_uploading = false;
        transfer_file_paths.clear();
    }
};

class ServerFTP {
public:
    explicit ServerFTP(int portCommand = 21);
    ~ServerFTP();
    void run();

private:
    CanalCommand commandServer_;
    Poller poller_;
    std::vector<FTPClient> clients;

    void handleNewConnection();
    void handleClientData(int fd);
    void handleClientDisconnection(int fd);
};
