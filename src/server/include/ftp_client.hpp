#pragma once

#include <string>
#include <memory>
#include "sslconnect.hpp"

enum class FTPMode {
    Undefined, // The mode is not set or is unknown.
    Active,    // The client opens a random port for data transfer.
    Passive    // The server opens a random port for data transfer.
};

enum class Type {
    A,  // ASCII
    I,  // Binary
    E,  // EBCDIC
    L   // Local
};

enum class DataProtectionLevel {
    None,         // No data protection is applied.
    Confidential, // Data is protected to maintain confidentiality.
    Security,     // Data is protected with a general security level.
    Encryption    // Data is protected using encryption.
};

struct FTPDataInfo {
    bool ssl_activate;
    SSL *ssl;
    DataProtectionLevel protection_level;

    int port_client;
    int server_socket;

    FTPMode mode;
    Type type;

    FTPDataInfo(FTPMode m = FTPMode::Undefined, int port = -1) :
        ssl_activate(false),
        ssl(nullptr),
        protection_level(DataProtectionLevel::None),
        port_client(port),
        server_socket(-1),
        mode(m),
        type(Type::A)
    {}
};

enum class TypeProtocole {
    FTP,   // Standard FTP protocol.
    FTPES, // FTP over a secured connection with SSL/TLS after connection (FTPES).
};

struct FTPClient {
    SSL_CTX *context;
    SSL *ssl = nullptr;
    int socket_fd;

    TypeProtocole type_protocole;

    bool is_secure;
    bool is_tls_negotiated;

    std::string client_adress_ip;
    std::string username;
    std::string current_directory;
    std::string last_command;

    bool authenticated;

    FTPDataInfo data_info;

    FTPClient(int fd = -1) :
        context(nullptr),
        ssl(nullptr),
        socket_fd(fd),
        type_protocole(TypeProtocole::FTP),
        is_secure(false),
        is_tls_negotiated(false),
        client_adress_ip(""),
        username(""),
        current_directory("/"),
        last_command(""),
        authenticated(false),
        data_info()
    {}
};
