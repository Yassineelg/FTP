#pragma once

#include "ftp_client.hpp"
#include "sslconnect.hpp"

class CanalData {
public:
    CanalData(FTPDataInfo *data, SSL_CTX *context);

    bool setupConnection();
    ssize_t sendData(const void* data, size_t size);
    ssize_t receiveData(void* buffer, size_t size);
    void closeConnection();
    bool acceptConnection();

    int serverSocket_;

private:

    bool initializeSSL();

    FTPDataInfo *data_;
    int clientSocket_;
    SSL_CTX *sslContext_;
    SSL *ssl_;
};
