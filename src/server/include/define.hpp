#pragma once

#include <string>

#define FTP_ROOT_DEFAULT_DIRECTORY_NAME "FTP_SERVER"
#if defined(__linux__)
#define FTP_ROOT_DIRECTORY "/srv/FTP_SERVER/"
#elif defined(__APPLE__) && defined(__MACH__)
#define FTP_ROOT_DIRECTORY "/Users/Shared/FTP_SERVER/"
#else
#error "Unsupported operating system"
#endif

#define FTP_DEFAULT_DIR_SERVER std::string(std::string(FTP_ROOT_DIRECTORY) + std::string("server/")).c_str()
#define FTP_DEFAULT_DIR_SSL std::string(std::string(FTP_DEFAULT_DIR_SERVER) + std::string("ssl/"))
#define FTP_DEFAULT_FILE_USERS std::string(std::string(FTP_DEFAULT_DIR_SERVER) + std::string("users")).c_str()
#define FTP_DEFAULT_DIR_USERS std::string(std::string(FTP_ROOT_DIRECTORY) + std::string("users/")).c_str()
#define FTP_DEFAULT_DIR_USER(user) std::string(std::string(FTP_DEFAULT_DIR_USERS) + std::string(user)).c_str()

#define FTP_DEFAULT_DIR_LOG std::string(std::string(FTP_ROOT_DIRECTORY) + std::string("log/")).c_str()
