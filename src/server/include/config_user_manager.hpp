#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

#include "define.hpp"

class ConfigUserManager {
public:
    void configUsersServer();
    void addUser(const std::string& username, const std::string& password);
    void removeUser(const std::string& username);
    void changePassword(const std::string& username, const std::string& newPassword);
    void displayMenu();
    bool handleUserInput();
};
