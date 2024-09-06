#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>

enum class LogLevel {
    FATAL = 0,
    ERROR = 1,
    WARN = 2,
    INFO = 3,
    DEBUG = 4,
    TRACE = 5
};

class Logger {
public:
    static void initialize(const std::string& logFile = "ftp_client.log", LogLevel level = LogLevel::INFO);
    static void log(LogLevel level, const std::string& message);

    static void fatal(const std::string& message);
    static void error(const std::string& message);
    static void warn(const std::string& message);
    static void info(const std::string& message);
    static void debug(const std::string& message);
    static void trace(const std::string& message);

private:
    static std::ofstream logFileStream;
    static LogLevel currentLogLevel;

    static std::string getTimestamp();
    static std::string levelToString(LogLevel level);
    static void writeToFileAndConsole(const std::string& logMessage);
};

#endif
