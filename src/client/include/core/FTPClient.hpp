#ifndef FTPCLIENT_HPP
#define FTPCLIENT_HPP

#include "service/NetworkService.hpp"
#include <string>

class FTPClient {
public:
    explicit FTPClient(NetworkService* networkService);
    [[nodiscard]] bool connect(const std::string& host, int port) const;
    [[nodiscard]] bool login(const std::string& username, const std::string& password) const;
    [[nodiscard]] std::string sendCommand(const std::string& command, const std::string& args = "") const;
    void close() const;

private:
    NetworkService* networkService;
    static std::string sanitizeResponse(const std::string& response);
};

#endif
