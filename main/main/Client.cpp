#include "Client.hpp"
#include "Discovery.hpp"  // Assumes discoverServer() is declared here.
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#endif

#define PORT 1235  // Port to connect to

using json = nlohmann::json;
using namespace std;

// Get the server IP using your discovery function.
string server_ip = discoverServer();

namespace Client {

    atomic<bool> client_running(true);
    int client_socket = -1;

    // Handles receiving messages from the server continuously.
    void handle_server(int client_socket) {
        char buffer[1024];
        while (client_running) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                cout << "Server says: " << buffer << "\n";
            }
            else if (bytes_received == 0) {
                cout << "Server disconnected.\n";
                client_running = false;
                break;
            }
            else {
#ifdef _WIN32
                int error = WSAGetLastError();
#else
                int error = errno;
#endif
                cerr << "Failed to receive data from server, error code: " << error << "\n";
                client_running = false;
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }

    bool notifyStartGame() {
        cout << "Attempting to notify server to start game on IP "
            << server_ip << " and port " << PORT << endl;
        if (client_socket < 0) {
            cerr << "Client socket is not connected!" << endl;
            return false;
        }
        string message = "START_GAME";
        if (send(client_socket, message.c_str(), message.size(), 0) < 0) {
#ifdef _WIN32
            int error = WSAGetLastError();
#else
            int error = errno;
#endif
            cerr << "Failed to send START_GAME message, error code: " << error << endl;
            return false;
        }
        return true;
    }

    // Serializes a tetromino’s block coordinates as JSON and sends it to the server.
    bool sendDragCoordinates(const Tetromino& tetromino) {
        if (client_socket < 0) {
            cerr << "Client socket is not connected!" << endl;
            return false;
        }
        json j;
        j["type"] = "DRAG_UPDATE";
        j["blocks"] = json::array();
        for (const auto& block : tetromino.blocks) {
            j["blocks"].push_back({ {"x", block.x}, {"y", block.y} });
        }
        string message = j.dump();

        // Use a loop to ensure the full message is sent.
        size_t totalSent = 0;
        size_t messageLength = message.size();
        while (totalSent < messageLength) {
            int sent = send(client_socket, message.c_str() + totalSent,
                messageLength - totalSent, 0);
            if (sent <= 0) {
#ifdef _WIN32
                int error = WSAGetLastError();
#else
                int error = errno;
#endif
                cerr << "Failed to send drag coordinates! Error code: " << error << endl;
                return false;
            }
            totalSent += sent;
        }
        cout << "Drag coordinates sent: " << message << endl;
        return true;
    }

    // Connects to the server given its IP address.
    void start_client(const string& server_ip) {
        cout << "*Server IP: " << server_ip << endl;
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "WSAStartup failed!" << endl;
            return;
        }
#endif
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            cerr << "Socket creation failed" << endl;
            return;
        }
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            cerr << "Invalid address" << endl;
            return;
        }
        cout << "Connecting to server..." << endl;
        if (connect(client_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            cerr << "Connection failed" << endl;
            return;
        }
        cout << "Connected to server" << endl;
        thread handle_thread(handle_server, client_socket);
        while (client_running) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        cout << "Disconnecting..." << endl;
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
        client_running = false;
        handle_thread.join();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // New: runClient() simply checks if the server IP was discovered and calls start_client().
    void runClient() {
        if (!server_ip.empty()) {
            start_client(server_ip);
        }
        else {
            cerr << "Could not find server automatically!" << endl;
        }
    }
}
