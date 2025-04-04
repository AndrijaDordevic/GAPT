#include <iostream>
#include <cstring>  // For memset
#include <cstdlib>  // For exit()
#include <thread>   // For multi-threading
#include <chrono>   // For sleep_for
#include <string>
#include "Client.hpp"
#include <atomic>
#include "Discovery.hpp"  // For discoverServer()
#include <string>
#include <fstream>


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

using namespace std;
string server_ip = discoverServer();

namespace Client{

    // Global atomic flag to manage client shutdown
    std::atomic<bool> client_running(true);

    int client_socket = -1;

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

    void runClient() {
        // Read the server IP from the file
        if (!server_ip.empty()) {
            Client::start_client(server_ip);
        }
        else {
            cerr << "Could not find server automatically!\n";
        }


        // If the IP is empty, print an error and stop the client
        if (server_ip.empty()) {
            cerr << "Client: No server IP found. Please ensure the listener has saved it." << endl;
            return;
        }

        // At this point, we have a valid server IP, so we can call start_client() with the IP
        cout << "Client: Using server IP: " << server_ip << "\n";

        // Start the client with the IP address
        Client::start_client(server_ip);  // Call your client start function here
    }

    bool Client::notifyStartGame() {
        std::cout << "Attempting to notify server to start game on IP " << server_ip << " and port " << PORT << std::endl;

        // Use the existing client socket, which is already connected
        if (client_socket < 0) {
            std::cerr << "Client socket is not connected!" << std::endl;
            return false;
        }

        // Send the START_GAME message
        std::string message = "START_GAME";
        if (send(client_socket, message.c_str(), message.size(), 0) < 0) {
            std::cerr << "Failed to send START_GAME message!" << std::endl;
            return false;
        }

        return true;
    }



    // Function to initiate a connection to the server
    void Client::start_client(const std::string& server_ip) {
        std::cout << "*Server IP:  " << server_ip << std::endl;

    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

        // Create socket
        client_socket = socket(AF_INET, SOCK_STREAM, 0); // Use global client_socket
        if (client_socket == -1) {
            std::cerr << "Socket creation failed\n";
            return;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);

        // Convert server IP address to binary
        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address\n";
            return;
        }

        // Connect to the server
        std::cout << "Connecting to server...\n";
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            std::cerr << "Connection failed\n";
            return;
        }

        std::cout << "Connected to server\n";

        // Start a thread to handle communication
        std::thread handle_thread(handle_server, client_socket);

        // Keep the client running
        while (client_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "Disconnecting...\n";
        closesocket(client_socket); // Use the global client_socket
        client_running = false;
        handle_thread.join();

    #ifdef _WIN32
        WSACleanup();
    #endif
    }

}