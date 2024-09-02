#include "../include/main.hpp"

void displayMenu() {
    std::cout << std::endl;
    std::cout << "1. Config Users" << std::endl;
    std::cout << "2. Run Server" << std::endl;
    std::cout << "3. Quit" << std::endl;
    std::cout << std::endl;
    std::cout << "Select Number: ";
}

int main() {
    std::cout << "==========================" << std::endl;
    std::cout << "        FTP Server        " << std::endl;
    std::cout << "==========================" << std::endl;
    while (true) {
        int choice;
        displayMenu();

        std::cin >> choice;
        std::cout << std::endl;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number." << std::endl;
            continue;
        }

        switch (choice) {
            case 1:
                ConfigUserManager manager;
                manager.configUsersServer();
                break;
            case 2: {
                ServerFTP server;
                server.run();
                return 0;
            }
            case 3:
                std::cout << "Exiting program." << std::endl;
                return 0;
            default:
                std::cout << "Invalid choice. Please select 1, 2, or 3." << std::endl;
                break;
        }
    }
    return 0;
}