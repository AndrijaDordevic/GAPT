#include "Listener.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <thread>

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

#define LISTEN_PORT 12345  // Must match the broadcast port

void listenForServerIP() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "Listener: UDP socket creation failed!" << std::endl;
        return;
    }

    sockaddr_in listener_addr;
    memset(&listener_addr, 0, sizeof(listener_addr));
    listener_addr.sin_family = AF_INET;
    listener_addr.sin_port = htons(LISTEN_PORT);
    listener_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket, (struct sockaddr*)&listener_addr, sizeof(listener_addr)) < 0) {
        std::cerr << "Listener: Binding failed!" << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
        return;
    }

    char buffer[256];
    sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    std::cout << "Listener: Waiting for server broadcast..." << std::endl;

    int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
        (struct sockaddr*)&sender_addr, &sender_len);
    if (bytes_received < 0) {
        std::cerr << "Listener: Failed to receive data!" << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
        return;
    }

    buffer[bytes_received] = '\0';  // Null-terminate the received message
    std::string server_ip = buffer;

    std::ofstream outfile("server_ip.txt");
    if (outfile) {
        outfile << server_ip;
        std::cout << "Listener: Server IP (" << server_ip << ") saved to server_ip.txt" << std::endl;
    }
    else {
        std::cerr << "Listener: Failed to write server IP to file!" << std::endl;
    }

#ifdef _WIN32
    closesocket(udp_socket);
    WSACleanup();
#else
    close(udp_socket);
#endif
}
