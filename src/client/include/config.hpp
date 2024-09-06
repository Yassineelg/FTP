#ifndef CONFIG_HPP
#define CONFIG_HPP

// Network Configuration
#define DEFAULT_FTP_PORT 21              // Default FTP port
#define MAX_BUFFER_SIZE 1024             // Maximum buffer size for reading from socket
#define SOCKET_TIMEOUT_SEC 5             // Timeout in seconds for socket operations

// Log Configuration
#define LOG_FILE "ftp_client.log"
#define LOG_PATH "./log/"
#define LOG_LEVEL LogLevel::INFO  // Default log level for the application

#endif
