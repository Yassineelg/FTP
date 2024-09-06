#ifndef NETWORKSERVICE_HPP
#define NETWORKSERVICE_HPP

#include <string>

class NetworkService {
public:
    NetworkService();
    ~NetworkService();

    bool connectToServer(const std::string& host, int port);
    void closeConnection();
    [[nodiscard]] std::string sendAndReceive(const std::string& data) const;
    [[nodiscard]] static std::string sanitizeResponse(const std::string& response);

private:
    int socketFd;
    [[nodiscard]] std::string readFromSocket() const;
    void writeToSocket(const std::string& data) const;
};

#endif
