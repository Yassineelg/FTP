#include "../include/config_user_manager.hpp"

void ConfigUserManager::addUser() {
    std::cout << "    Add a new user:" << std::endl;
    std::string username;
    std::string password;

    std::cout << "      Username: ";
    std::cin >> username;
    std::cout << "      Password: ";
    std::cin >> password;

    std::ofstream file(FTP_FILE_USERS, std::ios::app);
    if (file.is_open()) {
        file << username << ":" << password << std::endl;
        file.close();
        std::cout << "      User \"" << username << "\" added successfully." << std::endl;
    } else {
        std::cerr << "      Error: Unable to open file \"" << FTP_FILE_USERS << "\"." << std::endl;
        return;
    }

    std::filesystem::path userDir = std::filesystem::path(FTP_DIR_USER(username));
    try {
        if (!std::filesystem::exists(userDir)) {
            std::filesystem::create_directories(userDir);
            std::cout << "      Directory for user \"" << username << "\" created successfully." << std::endl;
        } else {
            std::cerr << "      Error: Directory for user \"" << username << "\" already exists." << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "      Error: " << e.what() << std::endl;
    }
}

void ConfigUserManager::removeUser() {
    std::cout << "    Remove a user:" << std::endl;
    std::string username;
    std::cout << "      Username: ";
    std::cin >> username;

    std::ifstream fileIn(FTP_FILE_USERS);
    if (!fileIn.is_open()) {
        std::cerr << "      Error: Unable to open file \"" << FTP_FILE_USERS << "\"." << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(fileIn, line)) {
        if (line.substr(0, line.find(':')) != username) {
            lines.push_back(line);
        }
    }
    fileIn.close();
    std::ofstream fileOut(FTP_FILE_USERS);
    if (!fileOut.is_open()) {
        std::cerr << "      Error: Unable to open file \"" << FTP_FILE_USERS << "\" for writing." << std::endl;
        return;
    }
    for (const auto& l : lines) {
        fileOut << l << std::endl;
    }
    fileOut.close();

    std::filesystem::path userDir = std::filesystem::path(FTP_DIR_USER(username));
    try {
        if (std::filesystem::exists(userDir)) {
            std::filesystem::remove_all(userDir);
            std::cout << "      User \"" << username << "\" and associated directory removed successfully." << std::endl;
        } else {
            std::cerr << "      Error: Directory for user \"" << username << "\" does not exist." << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "      Error: " << e.what() << std::endl;
    }
}

void ConfigUserManager::changePassword() {
    std::cout << "    Change password:" << std::endl;
    std::string username, newPassword;
    std::cout << "      Username: ";
    std::cin >> username;
    std::cout << "      New password: ";
    std::cin >> newPassword;

    std::ifstream fileIn(FTP_FILE_USERS);
    if (!fileIn.is_open()) {
        std::cerr << "    Error: Unable to open file \"" << FTP_FILE_USERS << "\"." << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    bool userFound = false;
    while (std::getline(fileIn, line)) {
        std::string currentUsername = line.substr(0, line.find(':'));
        if (currentUsername == username) {
            line = username + ":" + newPassword;
            userFound = true;
        }
        lines.push_back(line);
    }
    fileIn.close();

    if (!userFound) {
        std::cerr << "      Error: User \"" << username << "\" not found." << std::endl;
        return;
    }
    std::ofstream fileOut(FTP_FILE_USERS);
    if (!fileOut.is_open()) {
        std::cerr << "      Error: Unable to open file \"" << FTP_FILE_USERS << "\" for writing." << std::endl;
        return;
    }
    for (const auto& l : lines) {
        fileOut << l << std::endl;
    }
    fileOut.close();
    std::cout << "      Password for \"" << username << "\" changed successfully." << std::endl;
}

void ConfigUserManager::configUsersServer() {
    while (true) {
        std::cout << "\n  Server Configuration Users:" << std::endl;
        std::cout << "    1. Add a user" << std::endl;
        std::cout << "    2. Remove a user" << std::endl;
        std::cout << "    3. Change password" << std::endl;
        std::cout << "    4. Return to main menu\n" << std::endl;
        std::cout << "    Select number: ";

        int choice;
        std::cin >> choice;
        std::cout << std::endl;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "    Invalid input. Please enter a number." << std::endl;
            continue;
        }

        std::cout << std::endl;

        switch (choice) {
            case 1:
                addUser();
                break;
            case 2:
                removeUser();
                break;
            case 3:
                changePassword();
                break;
            case 4:
                return;
            default:
                std::cout << "\n    Invalid choice. Please select a valid number." << std::endl;
                break;
        }
    }
}
