#include <iostream>
#include <cstring>     // Pour strlen, strcpy
#include <string>

#ifdef _WIN32
#include <winsock2.h>  // Windows Sockets
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib

#define CLOSE_SOCKET closesocket
#define INIT_SOCKETS() do { \
    WSADATA wsaData; \
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { \
        std::cerr << "Erreur d'initialisation de Winsock" << std::endl; \
        return 1; \
    } \
} while(0)

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // Pour inet_pton
#include <unistd.h>    // Pour close

#define CLOSE_SOCKET close
#define INIT_SOCKETS() // Rien à faire pour les systèmes Unix
#endif

// Fonction pour recevoir les données du serveur
std::string receiveFromServer(int socket) {
    char buffer[1024];
    std::string response;
    ssize_t bytesReceived;

    // Réinitialiser le tampon
    std::memset(buffer, 0, sizeof(buffer));

    // Lire la réponse du serveur
    bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        std::cerr << "Erreur de réception des données" << std::endl;
    } else if (bytesReceived == 0) {
        std::cerr << "Le serveur a fermé la connexion" << std::endl;
    } else {
        response = std::string(buffer, bytesReceived);
    }

    return response;
}

int main() {
    const char* serverAddress = "127.0.0.1";
    int port = 21;

    INIT_SOCKETS();

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Erreur de création du socket" << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverAddress, &serverAddr.sin_addr) <= 0) {
        std::cerr << "Erreur de conversion de l'adresse IP" << std::endl;
        CLOSE_SOCKET(clientSocket);
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Erreur de connexion au serveur" << std::endl;
        CLOSE_SOCKET(clientSocket);
        return 1;
    }

    std::cout << "Connecté au serveur FTP." << std::endl;
    std::cout << "sur le port : " << port << std::endl;

    while (true) {
        std::string command;
        std::getline(std::cin, command);

        if (command == "QUIT") {
            std::cout << "Déconnexion du serveur." << std::endl;
            if (send(clientSocket, command.c_str(), command.length(), 0) < 0) {
                std::cerr << "Erreur d'envoi de la commande" << std::endl;
                CLOSE_SOCKET(clientSocket);
            }
            std::string response = receiveFromServer(clientSocket);
            std::cout << "recive : " << response << std::endl;
            break;
        }

        if (send(clientSocket, command.c_str(), command.length(), 0) < 0) {
            std::cerr << "Erreur d'envoi de la commande" << std::endl;
            CLOSE_SOCKET(clientSocket);
            return 1;
        }

        std::cout << "send   : " << command << std::endl;

        std::string response = receiveFromServer(clientSocket);
        std::cout << "recive : " << response << std::endl;
    }

    CLOSE_SOCKET(clientSocket);

#ifdef _WIN32
    WSACleanup(); // Nettoyage des sockets sous Windows
#endif

    return 0;
}
