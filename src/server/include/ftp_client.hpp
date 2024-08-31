#pragma once

#include <string>

enum class FTPMode {
    Undefined,
    Active,
    Passive
};

struct FTPDataInfo {
    FTPMode mode;
    int port_client;

    FTPDataInfo(FTPMode m = FTPMode::Undefined, int fd = -1, int port = -1)
        : mode(m), port_client(port) 
    {}
};

struct FTPClient {
    int socket_fd;
    std::string username;
    std::string current_directory;
    bool authenticated;
    FTPDataInfo* data_info;

    FTPClient(int fd = -1)
        : socket_fd(fd),
          username(""),
          current_directory("/"),
          authenticated(false),
          data_info(new FTPDataInfo()) 
    {}

    ~FTPClient() {
        delete data_info;
    }
};
