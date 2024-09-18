#include "config_user_manager.hpp"

static std::string generateSalt() {
    unsigned char salt[16];
    if (!RAND_bytes(salt, sizeof(salt))) {
        throw std::runtime_error("Unable to generate salt");
    }
    std::stringstream ss;
    for (int i = 0; i < sizeof(salt); ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)salt[i];
    }
    return ss.str();
}

static std::string hashPassword(const std::string& password, const std::string& salt) {
    std::string saltedPassword = password + salt;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) throw std::runtime_error("Unable to create EVP_MD_CTX");

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) throw std::runtime_error("Unable to initialize digest");
    if (1 != EVP_DigestUpdate(mdctx, saltedPassword.c_str(), saltedPassword.size())) throw std::runtime_error("Unable to update digest");
    if (1 != EVP_DigestFinal_ex(mdctx, hash, &hashLen)) throw std::runtime_error("Unable to finalize digest");

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)hash[i];
    }
    return ss.str();
}


void ConfigUserManager::addUser(const std::string& username, const std::string& password) {
    std::string salt = generateSalt();
    std::string hashedPassword = hashPassword(password, salt);

    std::ofstream file(FTP_DEFAULT_FILE_USERS, std::ios::app);
    if (file.is_open()) {
        file << username << ":" << hashedPassword << ":" << salt << std::endl;
        file.close();
        std::cout << "      User \"" << username << "\" added successfully." << std::endl;
    } else {
        std::cerr << "      Error: Unable to open file \"" << FTP_DEFAULT_FILE_USERS << "\"." << std::endl;
        return;
    }

    std::filesystem::path userDir = std::filesystem::path(FTP_DEFAULT_DIR_USER(username));
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

void ConfigUserManager::changePassword(const std::string& username, const std::string& newPassword) {
    std::ifstream fileIn(FTP_DEFAULT_FILE_USERS);
    if (!fileIn.is_open()) {
        std::cerr << "    Error: Unable to open file \"" << FTP_DEFAULT_FILE_USERS << "\"." << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    bool userFound = false;
    while (std::getline(fileIn, line)) {
        std::string currentUsername = line.substr(0, line.find(':'));
        if (currentUsername == username) {
            std::string salt = line.substr(line.rfind(':') + 1);
            std::string hashedPassword = hashPassword(newPassword, salt);
            line = username + ":" + hashedPassword + ":" + salt;
            userFound = true;
        }
        lines.push_back(line);
    }
    fileIn.close();

    if (!userFound) {
        std::cerr << "      Error: User \"" << username << "\" not found." << std::endl;
        return;
    }
    std::ofstream fileOut(FTP_DEFAULT_FILE_USERS);
    if (!fileOut.is_open()) {
        std::cerr << "      Error: Unable to open file \"" << FTP_DEFAULT_FILE_USERS << "\" for writing." << std::endl;
        return;
    }
    for (const auto& l : lines) {
        fileOut << l << std::endl;
    }
    fileOut.close();
    std::cout << "      Password for \"" << username << "\" changed successfully." << std::endl;
}

void ConfigUserManager::removeUser(const std::string& username) {
    std::ifstream fileIn(FTP_DEFAULT_FILE_USERS);
    if (!fileIn.is_open()) {
        std::cerr << "      Error: Unable to open file \"" << FTP_DEFAULT_FILE_USERS << "\"." << std::endl;
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
    std::ofstream fileOut(FTP_DEFAULT_FILE_USERS);
    if (!fileOut.is_open()) {
        std::cerr << "      Error: Unable to open file \"" << FTP_DEFAULT_FILE_USERS << "\" for writing." << std::endl;
        return;
    }
    for (const auto& l : lines) {
        fileOut << l << std::endl;
    }
    fileOut.close();

    std::filesystem::path userDir = std::filesystem::path(FTP_DEFAULT_DIR_USER(username));
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

void ConfigUserManager::displayMenu() {
    std::cout << "\n  Server Configuration Users:" << std::endl;
    std::cout << "    1. Add a user" << std::endl;
    std::cout << "    2. Remove a user" << std::endl;
    std::cout << "    3. Change password" << std::endl;
    std::cout << "    4. Return to main menu\n" << std::endl;
    std::cout << "    Select number: ";
}

bool ConfigUserManager::handleUserInput() {
    int choice;
    std::cin >> choice;
    std::cout << std::endl;

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "    Invalid input. Please enter a number." << std::endl;
        return false;
    }

    std::cout << std::endl;

    std::string username, password;
    switch (choice) {
    case 1:
        std::cout << "      Username: ";
        std::cin >> username;
        std::cout << "      Password: ";
        std::cin >> password;
        addUser(username, password);
        break;
    case 2:
        std::cout << "      Username: ";
        std::cin >> username;
        removeUser(username);
        break;
    case 3:
        std::cout << "      Username: ";
        std::cin >> username;
        std::cout << "      New password: ";
        std::cin >> password;
        changePassword(username, password);
        break;
    case 4:
        return true;
    default:
        std::cout << "\n    Invalid choice. Please select a valid number." << std::endl;
        break;
    }
    return false;
}

void ConfigUserManager::configUsersServer() {
    while (true) {
        displayMenu();
        if (handleUserInput()) {
            return;
        }
    }
}
