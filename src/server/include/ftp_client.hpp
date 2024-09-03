#pragma once

#include <string>
#include <memory>

enum class FTPMode {
    Undefined,
    Active,
    Passive,
};

enum class Type {
    A,  // Ascii
    I,  // Binary
};

struct FTPDataInfo {
    FTPMode mode;
    Type type;
    int port_client;

    FTPDataInfo(FTPMode m = FTPMode::Undefined, int port = -1)
        : mode(m), type(Type::A), port_client(port)
    {}
};

struct FTPClient {
    int socket_fd;
    std::string username;
    std::string current_directory;
    bool authenticated;
    FTPDataInfo data_info;

    FTPClient(int fd = -1)
        : socket_fd(fd),
          username(""),
          current_directory("/"),
          authenticated(false),
          data_info()
    {}
};