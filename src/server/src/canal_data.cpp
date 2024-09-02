#include "../include/canal_data.hpp"

CanalData::CanalData(FTPDataInfo *data)
    : data_(data), server_socket_(-1), client_socket_(-1) {}

CanalData::~CanalData() {
    closeConnection();
}

bool CanalData::setupConnection() {
    if (data_->mode == FTPMode::Passive) {
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(data_->port_client);

        if (bind(server_socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            close(server_socket_);
            return false;
        }

        if (listen(server_socket_, 1) < 0) {
            close(server_socket_);
            return false;
        }

        client_socket_ = accept(server_socket_, nullptr, nullptr);
        if (client_socket_ < 0) {
            close(server_socket_);
            return false;
        }

        close(server_socket_);
        return true;
    } else if (data_->mode == FTPMode::Active) {
        client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket_ < 0) {
            return false;
        }

        sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(data_->port_client);
        client_addr.sin_addr.s_addr = INADDR_ANY;

        if (connect(client_socket_, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
            close(client_socket_);
            return false;
        }

        return true;
    }

    return false;
}

ssize_t CanalData::sendData(const void* data, size_t size) {
    return write(client_socket_, data, size);
}

ssize_t CanalData::receiveData(void* buffer, size_t size) {
    return read(client_socket_, buffer, size);
}

void CanalData::closeConnection() {
    if (client_socket_ >= 0) {
        close(client_socket_);
        client_socket_ = -1;
    }
    data_->mode = FTPMode::Undefined;
    data_->port_client = -1;
}
