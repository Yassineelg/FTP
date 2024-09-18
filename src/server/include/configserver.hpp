#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP

#include <iostream>
#include <fstream>
#include <unordered_map>

#define FTP_DEFAULT_FILE_CONFIG "/Users/Shared/FTP_SERVER/server/config.conf"
#define DEFAULT_SERVER_NAME "Vortex FTP Server"
#define DEFAULT_FTP_BANNER "Welcome to My FTP Server!"
#define DEFAULT_PORT "21"
#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_MAX_CLIENTS "10"
#define DEFAULT_BUFFER_SIZE_COMMAND "1024"
#define DEFAULT_BUFFER_SIZE_DATA "4096"
#define DEFAULT_SSL true

#define IP_SERVER 127.0.0.1
#define IP_SERVER_FORMAT "127,0,0,1"


class ConfigServer {
public:
    enum ConfigKey {
        SERVER_NAME_,
        FTP_BANNER_,
        PORT_,
        SERVER_IP_,
        MAX_CLIENTS_,
        MAX_CLIENTS_SAME_CONNECT_,
        BUFFER_SIZE_COMMAND_,
        BUFFER_SIZE_DATA_,
        SSL_ACTIVATE_,
        CONFIG_KEY_COUNT
    };

    ConfigServer(const std::string& filePath = FTP_DEFAULT_FILE_CONFIG);

    void setConfigValue(ConfigKey key, const std::string& value);
    void saveConfig() const;
    template <typename T>
    T getConfigValue(ConfigKey key) const;

private:
    bool loadConfig();

    std::string filePath_;
    std::unordered_map<std::string, std::string> config_;

    std::string keys_[CONFIG_KEY_COUNT];
    std::string defaultValues_[CONFIG_KEY_COUNT];
};

template <typename T>
T ConfigServer::getConfigValue(ConfigKey key) const {
    static_assert(sizeof(T) == 0, "Unsupported type for getConfigValue");
    return T{};
}

template <>
inline std::string ConfigServer::getConfigValue<std::string>(ConfigKey key) const {
    if (key >= 0 && key < CONFIG_KEY_COUNT) {
        auto it = config_.find(keys_[key]);
        if (it != config_.end()) {
            return it->second;
        }
    }
    return "";
}

template <>
inline int ConfigServer::getConfigValue<int>(ConfigKey key) const {
    if (key >= 0 && key < CONFIG_KEY_COUNT) {
        auto it = config_.find(keys_[key]);
        if (it != config_.end()) {
            return std::stoi(it->second);
        }
    }
    return 0;
}

#endif // CONFIGSERVER_HPP
