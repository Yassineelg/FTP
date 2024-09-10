#include "cli/CLI.hpp"
#include "core/FTPClient.hpp"
#include "service/NetworkService.hpp"
#include "service/Logger.hpp"
#include "./../include/config.hpp"

int main() {
    Logger::initialize(std::string(LOG_PATH) + LOG_FILE, LogLevel::DEBUG);

    NetworkService networkService;
    FTPClient ftpClient(&networkService);

    CommandHandler commandHandler(&ftpClient);
    const CLI cli(&ftpClient, &commandHandler);

    cli.start();
    return 0;
}
