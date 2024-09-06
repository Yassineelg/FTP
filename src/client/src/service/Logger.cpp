#include "./../../include/service/Logger.hpp"
#include "./../../include/Color.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <regex>

// Initialize static members
std::ofstream Logger::logFileStream;
LogLevel Logger::currentLogLevel = LogLevel::INFO;

void Logger::initialize(const std::string& logFile, const LogLevel level) {
    currentLogLevel = level;
    logFileStream.open(logFile, std::ios::out | std::ios::app);  // Append mode
    if (!logFileStream) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
        exit(1);
    }
}

void Logger::log(const LogLevel level, const std::string& message) {
    if (level > currentLogLevel) {
        return;  // Skip logging if the level is below the current log level
    }

    const std::string logMessage = "[" + getTimestamp() + "] " + levelToString(level) + ": " + RESET + message;

    // Write the log to both the file and the console
    writeToFileAndConsole(logMessage);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

std::string Logger::getTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

std::string Logger::levelToString(const LogLevel level) {
    switch (level) {
        case LogLevel::FATAL: return std::string(RED) + "FATAL";
        case LogLevel::ERROR: return std::string(BOLD) + RED + "ERROR";
        case LogLevel::WARN:  return std::string(YELLOW) + "WARN";
        case LogLevel::INFO:  return std::string(GREEN) + "INFO";
        case LogLevel::DEBUG: return std::string(PURPLE) + "DEBUG";
        case LogLevel::TRACE: return std::string(BLUE) + "TRACE";
        default: return "UNKNOWN";
    }
}

std::string removeColorCodes(const std::string& logMessage) {
    const std::regex colorCodeRegex("\033\\[[0-9;]*m");
    return std::regex_replace(logMessage, colorCodeRegex, "");
}

void Logger::writeToFileAndConsole(const std::string& logMessage) {
    std::cout << logMessage << std::endl;  // Print to console
    if (logFileStream.is_open()) {
        const std::string sanitizedMessage = removeColorCodes(logMessage);
        logFileStream << sanitizedMessage << std::endl;  // Write sanitized message to file
    }
}
