#pragma once

#include "ftp_client.hpp"

#include <cstddef>
#include <sys/types.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cerrno>
#include <stdexcept>

class FTPDataInfo;

class CanalData {
public:
    CanalData(FTPDataInfo *data);
    ~CanalData();

    bool setupConnection();
    ssize_t sendData(const void* data, size_t size);
    ssize_t receiveData(void* buffer, size_t size);
    void closeConnection();

private:
    FTPDataInfo *data_;
    int server_socket_;
    int client_socket_; 
};