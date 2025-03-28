#include <iostream>
#include <cstring>  // For memset
#include <cstdlib>  // For exit()
#include <thread>   // For multi-threading
#include <chrono>   // For sleep_for
#include <string>
#include "Client.hpp"
#include <atomic>
#include "IPReader.hpp"


#ifdef _WIN32  // Windows-specific headers
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // Link with Windows sockets library
#else  // Linux/macOS headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>  // For close()
#endif

#define PORT 1235  // Port to connect to

// Global atomic flag to manage client shutdown
std::atomic<bool> client_running(true);

// Function to handle receiving messages from the server
void handle_server(int client_socket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));  // Clear buffer

    // Receive a message from the server
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Ensure null termination
        std::cout << "Server says: " << buffer;  // Print server message
    }
    else if (bytes_received == 0) {
        std::cout << "Server disconnected.\n";
    }
    else {
        std::cerr << "Failed to receive data from server\n";
    }
}

// Function to initiate a connection to the server
void start_client(const std::string& server_ip) {

    std::cout << "*Server IP:  " << server_ip << std::endl;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int client_fd;
    struct sockaddr_in server_addr;

    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        std::cerr << "Socket creation failed\n";
        return;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert server IP address to binary
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return;
    }

    // Connect to the server
    std::cout << "Connecting to server...\n";
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Connection failed\n";
        return;
    }

    std::cout << "Connected to server\n";

    // Start a thread to handle communication
    std::thread handle_thread(handle_server, client_fd);

    // Keep the client running
    while (client_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Disconnecting...\n";
    closesocket(client_fd);
    client_running = false;
    handle_thread.join();

#ifdef _WIN32
    WSACleanup();
#endif
}