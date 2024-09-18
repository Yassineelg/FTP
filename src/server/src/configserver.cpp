#include "configserver.hpp"

ConfigServer::ConfigServer(const std::string& filePath) : filePath_(filePath) {

    keys_[SERVER_NAME_] = "server_name";
    keys_[FTP_BANNER_] = "ftp_banner";
    keys_[PORT_] = "port";
    keys_[SERVER_IP_] = "server_ip";
    keys_[MAX_CLIENTS_] = "max_clients";
    keys_[BUFFER_SIZE_COMMAND_] = "buffer_size_command";
    keys_[BUFFER_SIZE_DATA_] = "buffer_size_data";
    keys_[SSL_ACTIVATE_] = "ssl_activate";

    defaultValues_[SERVER_NAME_] = DEFAULT_SERVER_NAME;
    defaultValues_[FTP_BANNER_] = DEFAULT_FTP_BANNER;
    defaultValues_[PORT_] = DEFAULT_PORT;
    defaultValues_[SERVER_IP_] = DEFAULT_SERVER_IP;
    defaultValues_[MAX_CLIENTS_] = DEFAULT_MAX_CLIENTS;
    defaultValues_[BUFFER_SIZE_COMMAND_] = DEFAULT_BUFFER_SIZE_COMMAND;
    defaultValues_[BUFFER_SIZE_DATA_] = DEFAULT_BUFFER_SIZE_DATA;
    defaultValues_[SSL_ACTIVATE_] = DEFAULT_SSL;

    if (!loadConfig()) {
        for (int i = 0; i < CONFIG_KEY_COUNT; ++i) {
            config_[keys_[i]] = defaultValues_[i];
        }
        saveConfig();
    }
}

void ConfigServer::setConfigValue(ConfigKey key, const std::string& value) {
    if (key >= 0 && key < CONFIG_KEY_COUNT) {
        config_[keys_[key]] = value;
    }
    saveConfig();
}

void ConfigServer::saveConfig() const {
    std::ofstream outFile(filePath_);
    if (outFile.is_open()) {
        for (const auto& entry : config_) {
            outFile << entry.first << "=" << entry.second << "\n";
        }
        outFile.close();
    } else {
        std::cerr << "Error: Could not open file for writing: " << filePath_ << std::endl;
    }
}

bool ConfigServer::loadConfig() {
    std::ifstream inFile(filePath_);
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                config_[key] = value;
            }
        }
        inFile.close();
        return true;
    } else {
        return false;
    }
}
