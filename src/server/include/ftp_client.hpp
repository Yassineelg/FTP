#pragma once

#include <vector>

enum class FTPMode {
    Actif,
    Passif
};

struct FTPClient {
    int socket_fd;
    std::string username;
    std::string current_directory;
    bool authenticated;
    bool is_uploading;
    FTPMode is_passive;

    FTPClient(int fd = -1)
        : socket_fd(fd), 
          username(""),
          current_directory("/"),
          authenticated(false),
          is_uploading(false),
          is_passive(FTPMode::Passif) {}
};
  