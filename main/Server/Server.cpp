#include <iostream>
#include <cstring>  // For memset
#include <cstdlib>  // For exit()
#include <thread>   // For multi-threading
#include <vector>   // To store client sockets

#ifdef _WIN32  // Windows-specific headers
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else  // Linux/macOS headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define PORT 12345      // Port to listen on
#define MAX_CLIENTS 10  // Maximum number of clients

std::vector<int> clients;  // Store connected client sockets

void handle_client(int client_socket) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        // Receive data
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cout << "Client disconnected\n";
            break;
        }

        std::cout << "Client says: " << buffer << "\n";

        // TODO: Update game state, validate move

        // Echo message back to client
        send(client_socket, buffer, bytes_received, 0);
    }

#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int server_fd;
    struct sockaddr_in server_addr;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Binding failed\n";
        return -1;
    }

    // Start listening
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        std::cerr << "Listening failed\n";
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Accept client connection
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            std::cerr << "Client accept failed\n";
            continue;
        }

        std::cout << "New client connected!\n";

        // Store client socket
        clients.push_back(client_socket);

        // Handle client in a new thread
        std::thread(handle_client, client_socket).detach();
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif

    return 0;
}
