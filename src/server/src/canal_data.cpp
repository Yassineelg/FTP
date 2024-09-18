#include "canal_data.hpp"

CanalData::CanalData(FTPDataInfo *data, SSL_CTX *context)
    : data_(data), clientSocket_(-1), serverSocket_(-1), sslContext_(context), ssl_(nullptr) {}

bool CanalData::initializeSSL() {
    if (data_->ssl == nullptr && sslContext_) {
        data_->ssl = SSL_new(sslContext_);
        if (data_->ssl == nullptr) {
            std::cerr << "Error: Failed to create SSL object." << std::endl;
            return false;
        }

        SSL_set_fd(data_->ssl, clientSocket_);

        if (SSL_accept(data_->ssl) <= 0) {
            std::cerr << "Error: SSL handshake failed." << std::endl;
            SSL_free(data_->ssl);
            data_->ssl = nullptr;
            return false;
        }
    }
    return true;
}

bool CanalData::acceptConnection() {
    if ((clientSocket_ = accept(serverSocket_, nullptr, nullptr)) < 0) {
        std::cerr << "Error: Failed to accept client connection." << std::endl;
        close(serverSocket_);
        return false;
    }

    if (data_->ssl_activate) {
        if (!initializeSSL()) {
            std::cerr << "Error: Failed to initialize SSL." << std::endl;
            close(clientSocket_);
            clientSocket_ = -1;
            return false;
        }
    }
    return true;
}

bool CanalData::setupConnection() {
    if (data_->mode == FTPMode::Undefined) {
        serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket_ < 0) {
            std::cerr << "Error: Failed to create server socket." << std::endl;
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(data_->port_client);

        if (bind(serverSocket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Error: Failed to bind server socket." << std::endl;
            close(serverSocket_);
            return false;
        }

        if (listen(serverSocket_, 1) < 0) {
            std::cerr << "Error: Failed to listen on server socket." << std::endl;
            close(serverSocket_);
            return false;
        }

        data_->server_socket = serverSocket_;
        return true;
    } else if (data_->mode == FTPMode::Active) {
        clientSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket_ < 0) {
            std::cerr << "Error: Failed to create client socket." << std::endl;
            return false;
        }

        sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(data_->port_client);
        client_addr.sin_addr.s_addr = INADDR_ANY;

        if (connect(clientSocket_, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
            std::cerr << "Error: Failed to connect to server." << std::endl;
            close(clientSocket_);
            return false;
        }

        if (data_->ssl_activate) {
            if (!initializeSSL()) {
                std::cerr << "Error: Failed to initialize SSL." << std::endl;
                close(clientSocket_);
                clientSocket_ = -1;
                return false;
            }
        }

        return true;
    } else if (data_->mode == FTPMode::Passive) {
        serverSocket_ = data_->server_socket;
        return true;
    }
    return false;
}

ssize_t CanalData::sendData(const void* data, size_t size) {
    if (data_->ssl_activate) {
        return SSL_write(data_->ssl, data, size);
    } else {
        return write(clientSocket_, data, size);
    }
}

ssize_t CanalData::receiveData(void* buffer, size_t size) {
    if (data_->ssl_activate) {
        return SSL_read(data_->ssl, buffer, size);
    } else {
        return read(clientSocket_, buffer, size);
    }
}

void CanalData::closeConnection() {
    if (clientSocket_ >= 0) {
        if (data_->ssl_activate && data_->ssl) {
            SSL_shutdown(data_->ssl);
            SSL_free(data_->ssl);
            data_->ssl = nullptr;
        }
        close(clientSocket_);
        clientSocket_ = -1;
    }

    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    data_->mode = FTPMode::Undefined;
    data_->port_client = -1;
}
