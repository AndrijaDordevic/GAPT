#include "Discovery.hpp"
#include <iostream>
#include <fstream>      
#include <cstring>

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

#define PORT 1235

std::string discoverServer() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return "";
    }
#endif

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "Failed to create UDP socket: ";
#ifdef _WIN32
        std::cerr << WSAGetLastError();
#else
        std::cerr << strerror(errno);
#endif
        std::cerr << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return "";
    }

    int reuse = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
        std::cerr << "Failed to set SO_REUSEADDR: ";
#ifdef _WIN32
        std::cerr << WSAGetLastError();
#else
        std::cerr << strerror(errno);
#endif
        std::cerr << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
        WSACleanup();
#else
        close(udp_socket);
#endif
        return "";
    }

    sockaddr_in listen_addr{};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(PORT);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        std::cerr << "Binding UDP socket failed: ";
#ifdef _WIN32
        std::cerr << WSAGetLastError();
#else
        std::cerr << strerror(errno);
#endif
        std::cerr << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
        WSACleanup();
#else
        close(udp_socket);
#endif
        return "";
    }

    char buffer[1024];
    sockaddr_in sender_addr{};
    socklen_t sender_len = sizeof(sender_addr);

    std::cout << "Waiting for server broadcast...\n";

#ifdef _WIN32
    DWORD timeout = 10000;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
        (struct sockaddr*)&sender_addr, &sender_len);

    std::string result;
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << "Received: " << buffer << std::endl;

        std::string received_data(buffer);
        if (received_data.find("SERVER_IP:") == 0) {
            size_t first_colon = received_data.find(":");
            size_t second_colon = received_data.find(":", first_colon + 1);
            if (first_colon != std::string::npos && second_colon != std::string::npos) {
                result = received_data.substr(first_colon + 1, second_colon - first_colon - 1);
            }
        }
    }
    else {
        std::cerr << "No server broadcast received within timeout period\n";
    }

#ifdef _WIN32
    closesocket(udp_socket);
    WSACleanup();
#else
    close(udp_socket);
#endif

    // Save the discovered IP to a file for later retrieval
    if (!result.empty()) {
        std::ofstream outfile("server_ip.txt");
        if (outfile.is_open()) {
            outfile << result;
            outfile.close();
            std::cout << "Server IP saved to file: " << result << std::endl;
        }
        else {
            std::cerr << "Failed to open file for writing the server IP\n";
        }
    }

    return result;
}