#include <iostream>
#include <cstring>  // For memset
#include <cstdlib>  // For exit()
#include <thread>   // For multi-threading
#include <vector>   // To store client sockets

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

#define PORT 12345      // Port the server listens on
#define MAX_CLIENTS 10  // Maximum number of simultaneous clients

std::vector<int> clients;  // Store connected client sockets

// Function to dynamically retrieve the local IP address
std::string getLocalIP() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        return "";
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);  // Port 0 means any available port
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);  // Google DNS server as an external address

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Connection failed!" << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return "";
    }

    sockaddr_in name;
    socklen_t len = sizeof(name);
    if (getsockname(sock, (struct sockaddr*)&name, &len) == -1) {
        std::cerr << "Failed to get local IP address!" << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return "";
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &name.sin_addr, ip_str, sizeof(ip_str));

    return std::string(ip_str);
}

// Function to handle communication with a client
void handle_client(int client_socket, const std::string& server_ip) {
    char buffer[1024];

    // Send the server's IP address to the client
    std::string msg = server_ip + "\n";
    send(client_socket, msg.c_str(), msg.length(), 0);

    while (true) {
        memset(buffer, 0, sizeof(buffer));  // Clear the buffer

        // Receive data from the client
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cout << "Client disconnected\n";
            break;  // Exit the loop if the client disconnects
        }

        buffer[bytes_received] = '\0';  // Null-terminate the string
        std::cout << "Client says: " << buffer << "\n";

        // Echo the message back to the client
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
    WSAStartup(MAKEWORD(2, 2), &wsaData);  // Initialize Windows networking API
#endif

    int server_fd;
    struct sockaddr_in server_addr;

    // Get the server's local IP address
    std::string localIP = getLocalIP();
    if (!localIP.empty()) {
        std::cout << "Server's local IP address: " << localIP << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve local IP address!" << std::endl;
        return -1;
    }

    // Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    // Configure server address settings
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any address
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Binding failed\n";
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        std::cerr << "Listening failed\n";
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Accept new client connections
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            std::cerr << "Client accept failed\n";
            continue;  // Keep accepting clients even if one fails
        }

        std::cout << "New client connected!\n";

        // Start a new thread to handle the client
        std::thread(handle_client, client_socket, localIP).detach();
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif

    return 0;
}
