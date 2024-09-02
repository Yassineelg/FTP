#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <limits>
#include "define.hpp"

class ConfigUserManager {
private:
    void addUser();
    void removeUser();
    void changePassword();

public:
    void configUsersServer();
};