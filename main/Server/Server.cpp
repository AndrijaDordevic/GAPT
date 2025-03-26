#include <iostream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>

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

#define PORT 12345          // Port used for broadcasting the IP
#define CONTROL_PORT 12346  // Port used for control messages (e.g., STOP command)
#define BROADCAST_INTERVAL 1  // Seconds between broadcasts

std::atomic<bool> stopBroadcast{ false };

// Function to retrieve the local IP address
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

// Thread function to broadcast the IP continuously until stopped
void broadcastIP(const std::string& ip) {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "UDP socket creation failed!" << std::endl;
        return;
    }

    // Enable broadcasting option
    int broadcastEnable = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) < 0) {
        std::cerr << "Failed to set socket option for broadcast!" << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
        return;
    }

    sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;  // 255.255.255.255

    while (!stopBroadcast.load()) {
        int sent_bytes = sendto(udp_socket, ip.c_str(), ip.length(), 0,
            (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        if (sent_bytes < 0) {
            std::cerr << "Broadcast send failed!" << std::endl;
        }
        else {
            std::cout << "Broadcasted IP (" << ip << ") to the local network.\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
    }

#ifdef _WIN32
    closesocket(udp_socket);
#else
    close(udp_socket);
#endif
}

// Thread function to listen for a STOP command on CONTROL_PORT
void controlListener() {
    int ctrl_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctrl_socket < 0) {
        std::cerr << "Control socket creation failed!" << std::endl;
        return;
    }

    sockaddr_in ctrl_addr;
    memset(&ctrl_addr, 0, sizeof(ctrl_addr));
    ctrl_addr.sin_family = AF_INET;
    ctrl_addr.sin_port = htons(CONTROL_PORT);
    ctrl_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(ctrl_socket, (struct sockaddr*)&ctrl_addr, sizeof(ctrl_addr)) < 0) {
        std::cerr << "Binding control socket failed!" << std::endl;
#ifdef _WIN32
        closesocket(ctrl_socket);
#else
        close(ctrl_socket);
#endif
        return;
    }

    char buffer[1024];
    while (!stopBroadcast.load()) {
        memset(buffer, 0, sizeof(buffer));
        socklen_t addr_len = sizeof(ctrl_addr);
        int bytes_received = recvfrom(ctrl_socket, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr*)&ctrl_addr, &addr_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            if (std::string(buffer) == "STOP") {
                std::cout << "Received STOP command. Stopping broadcast.\n";
                stopBroadcast.store(true);
                break;
            }
        }
    }

#ifdef _WIN32
    closesocket(ctrl_socket);
#else
    close(ctrl_socket);
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

    // Start the broadcaster and control listener threads
    std::thread broadcaster(broadcastIP, localIP);
    std::thread ctrlThread(controlListener);

    // Setup server TCP listener
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);  // The same port client uses
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Binding failed\n";
        return -1;
    }

    if (listen(server_fd, 1) < 0) {
        std::cerr << "Listen failed\n";
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // Accept incoming connection from the client
    int client_socket;
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client_socket < 0) {
        std::cerr << "Connection failed\n";
        return -1;
    }

    std::cout << "Client connected\n";

    // Handle the client connection
    // You can now add code here to handle communication with the client

    // Wait for the threads to finish (they will end after receiving STOP)
    broadcaster.join();
    ctrlThread.join();

#ifdef _WIN32
    closesocket(server_fd);
#else
    close(server_fd);
#endif

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
