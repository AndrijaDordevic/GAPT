#include <iostream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define PORT 1235             // Port for broadcasting and TCP listening
#define TCP_PORT 12346        // Port for TCP connections (if needed separately)
#define CONTROL_PORT 12346      // Port for control messages (e.g., STOP command)
#define BROADCAST_INTERVAL 1  // Seconds between broadcasts

std::atomic<bool> stopBroadcast{ false };
std::atomic<int> clientIDCounter{ 0 };

std::string getLocalIP() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        return "";
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);  // Any available port
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);  // Use Google's DNS as destination

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

void broadcastIP(const std::string& ip) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!\n";
        return;
    }
#endif

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "UDP socket creation failed!\n";
        return;
    }

    int broadcastEnable = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) < 0) {
        std::cerr << "Failed to set broadcast option!" << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
        return;
    }

    sockaddr_in broadcast_addr{};
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    std::string message = "SERVER_IP:" + ip + ":" + std::to_string(PORT);

    while (!stopBroadcast.load()) {
        int sent_bytes = sendto(udp_socket, message.c_str(), message.size(), 0,
            (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        if (sent_bytes < 0) {
            std::cerr << "Broadcast failed!" << std::endl;
        }
        else {
            std::cout << "Broadcasting: " << message << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
    }

#ifdef _WIN32
    closesocket(udp_socket);
#else
    close(udp_socket);
#endif
}

// Function to handle each client connection
void handleClient(int client_socket, int clientID) {
    std::string idMessage = "Your unique client ID is: " + std::to_string(clientID) + "\n";
    send(client_socket, idMessage.c_str(), idMessage.size(), 0);
    std::cout << "Sent ID " << clientID << " to client." << std::endl;

    // Here you can add additional communication handling with the client.
    // For now, we'll simply sleep for a while and then close the connection.
    std::this_thread::sleep_for(std::chrono::seconds(10));

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

    std::string localIP = getLocalIP();
    if (localIP.empty()) {
        std::cerr << "Failed to retrieve local IP address!" << std::endl;
        return -1;
    }
    std::cout << "Server's local IP address: " << localIP << std::endl;

    // Start the broadcaster thread
    std::thread broadcaster(broadcastIP, localIP);

    // Setup server TCP listener
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);  // Using the same port for TCP listening
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return -1;
    }

    if (listen(server_fd, 10) < 0) { // Increase backlog if expecting multiple clients
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // Accept multiple clients in a loop
    std::vector<std::thread> clientThreads;
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            std::cerr << "Connection failed" << std::endl;
            continue;
        }

        // Generate a unique ID for this client
        int uniqueClientID = clientIDCounter.fetch_add(1) + 1;
        std::cout << "Client connected with unique ID: " << uniqueClientID << std::endl;

        // Spawn a new thread to handle this client
        clientThreads.emplace_back(std::thread(handleClient, client_socket, uniqueClientID));
    }

    // Wait for all client threads to finish before exiting
    for (auto& t : clientThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    stopBroadcast.store(true);
    broadcaster.join();

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif

    return 0;
}
