#pragma once

#include <string>

#define FTP_ROOT_DIRECTORY_NAME "FTP_SERVER"
#if defined(__linux__)
    #define FTP_ROOT_DIRECTORY "/srv/FTP_SERVER/"
#elif defined(__APPLE__) && defined(__MACH__)
    #define FTP_ROOT_DIRECTORY "/Users/Shared/FTP_SERVER/"
#else
    #error "Unsupported operating system"
#endif
#define FTP_DIR_CONFIG std::string(std::string(FTP_ROOT_DIRECTORY) + std::string("config/")).c_str()
#define FTP_DIR_USERS std::string(std::string(FTP_ROOT_DIRECTORY) + std::string("users/")).c_str()
#define FTP_DIR_LOG std::string(std::string(FTP_ROOT_DIRECTORY) + std::string("log/")).c_str()
#define FTP_DIR_USER(user) std::string(std::string(FTP_DIR_USERS) + std::string(user) + std::string("/")).c_str()

#define FTP_FILE_USERS std::string(std::string(FTP_DIR_USERS) + std::string("users")).c_str()
#define FTP_FILE_CONFIG std::string(std::string(FTP_DIR_CONFIG) + std::string("config")).c_str()

#define MAX_CLIENTS 10
#define MAX_THREAD_CLIENTS 10
#define MAX_THREAD MAX_CLIENTS * MAX_THREAD_CLIENTS / 2

#define IP_SERVER 127.0.0.1
#define IP_SERVER_FORMAT "127,0,0,1"

#define BUFFER_SIZE_DATA 4096
#define BUFFER_SIZE_COMMAND 1024

