#ifndef SSL_CONNECT_HPP
#define SSL_CONNECT_HPP

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

class SslConnect {
private:
    SSL_CTX *ctx;

public:
    SslConnect();
    ~SslConnect();

    bool initialize();
    bool configure(const char* cert_file, const char* key_file);
    SSL* acceptConnection(int client_socket);
    SSL_CTX *getContext();

    int sslSend(SSL* ssl, const char* message);
    int sslReceive(SSL* ssl, char* buffer, int size);

    void closeConnection(SSL* ssl);
    void cleanup();
};

#endif // SSL_CONNECT_HPP
