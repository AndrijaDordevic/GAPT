#include <iostream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <vector>
#include <mutex>
#include <queue>
#include <utility>
#include <algorithm>  // For std::remove

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
#define BROADCAST_INTERVAL 1  // Seconds between broadcasts

// Global atomic flag for broadcaster thread stop
std::atomic<bool> stopBroadcast(false);
// Global client ID counter
std::atomic<int> clientIDCounter(0);

// Thread-safe waiting queue for clients waiting to start a session.
// Each pair contains: { client_socket, clientID }
std::mutex waitingMutex;
std::queue< std::pair<int, int> > waitingClients;

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
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);  // Using Google's DNS as destination

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

// Function to broadcast the server IP address (for clients to discover the server)
void broadcastIP(const std::string& ip) {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "UDP socket creation failed!\n";
        return;
    }

    int broadcastEnable = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST,
        reinterpret_cast<char*>(&broadcastEnable), sizeof(broadcastEnable)) < 0) {
        std::cerr << "Failed to set broadcast option!" << std::endl;
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
    if (inet_pton(AF_INET, "255.255.255.255", &broadcast_addr.sin_addr) <= 0) {
        std::cerr << "Invalid broadcast address" << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
        return;
    }

    std::string message = "SERVER_IP:" + ip + ":" + std::to_string(PORT);
    while (!stopBroadcast.load()) {
        int sent_bytes = sendto(udp_socket, message.c_str(), message.size(), 0,
            reinterpret_cast<struct sockaddr*>(&broadcast_addr),
            sizeof(broadcast_addr));
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

// Session handler: handles communication between two paired clients.
// Each session is strictly limited to 2 clients.
void sessionHandler(int clientSocket1, int clientID1, int clientSocket2, int clientID2) {
    // Notify both clients that they are now in a session.
    std::string sessionMessage1 = "You are now in a session with client " + std::to_string(clientID2) + "\n";
    std::string sessionMessage2 = "You are now in a session with client " + std::to_string(clientID1) + "\n";
    send(clientSocket1, sessionMessage1.c_str(), sessionMessage1.size(), 0);
    send(clientSocket2, sessionMessage2.c_str(), sessionMessage2.size(), 0);
    std::cout << "Session started between client " << clientID1 << " and client " << clientID2 << std::endl;

    // Example: echo messages between clients.
    char buffer[1024];
    bool sessionActive = true;
    while (sessionActive) {
        // Read from clientSocket1
        int bytesRead = recv(clientSocket1, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string message = "Client " + std::to_string(clientID1) + ": " + buffer;
            send(clientSocket2, message.c_str(), message.size(), 0);
        }
        else {
            sessionActive = false;
        }

        // Read from clientSocket2
        bytesRead = recv(clientSocket2, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string message = "Client " + std::to_string(clientID2) + ": " + buffer;
            send(clientSocket1, message.c_str(), message.size(), 0);
        }
        else {
            sessionActive = false;
        }
    }

    // Clean up the client sockets when the session ends.
#ifdef _WIN32
    closesocket(clientSocket1);
    closesocket(clientSocket2);
#else
    close(clientSocket1);
    close(clientSocket2);
#endif
    std::cout << "Session between client " << clientID1 << " and client " << clientID2 << " ended." << std::endl;
}

// Dedicated client handler: continuously listens for messages from the client.
// When it receives "START_GAME", it either pairs the client with a waiting one or adds it to the waiting queue.
void clientHandler(int client_socket, int clientID) {
    char buffer[256];
    while (true) {
        int bytesRead = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            std::cerr << "Client " << clientID << " disconnected or an error occurred." << std::endl;
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
            return;
        }
        buffer[bytesRead] = '\0';
        std::string message(buffer);

        // Remove newline characters (if any)
        message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
        message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());

        if (message == "START_GAME") {
            std::cout << "Client " << clientID << " requested to start a game." << std::endl;
            {
                std::lock_guard<std::mutex> lock(waitingMutex);
                // If there is a waiting client, pair them into a session.
                if (!waitingClients.empty()) {
                    std::pair<int, int> waitingPair = waitingClients.front();
                    waitingClients.pop();

                    int otherSocket = waitingPair.first;
                    int otherID = waitingPair.second;

                    // Send the "START_GAME" notification to both clients.
                    std::string startMsg = "START_GAME";
                    send(otherSocket, startMsg.c_str(), startMsg.size(), 0);
                    send(client_socket, startMsg.c_str(), startMsg.size(), 0);

                    // Launch a new session thread for these two clients.
                    std::thread sessionThread(sessionHandler, otherSocket, otherID, client_socket, clientID);
                    sessionThread.detach();
                    std::cout << "Paired client " << otherID << " with client " << clientID << std::endl;
                }
                else {
                    // Otherwise, add this client to the waiting queue.
                    waitingClients.push(std::make_pair(client_socket, clientID));
                    std::string waitMsg = "Waiting for another client to join...\n";
                    send(client_socket, waitMsg.c_str(), waitMsg.size(), 0);
                }
            }
            // Exit the loop after processing the START_GAME command.
            break;
        }
        else {
            std::cerr << "Client " << clientID << " sent an unknown command: " << message << std::endl;
        }
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return -1;
    }
#endif

    std::string localIP = getLocalIP();
    if (localIP.empty()) {
        std::cerr << "Failed to retrieve local IP address!" << std::endl;
        return -1;
    }
    std::cout << "Server's local IP address: " << localIP << std::endl;

    // Start the broadcaster thread (for client discovery)
    std::thread broadcaster(broadcastIP, localIP);

    // Set up the server TCP listener.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return -1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    // Main loop: Accept incoming client connections.
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            std::cerr << "Connection failed" << std::endl;
            continue;
        }

        // Assign a unique client ID.
        int uniqueClientID = clientIDCounter.fetch_add(1) + 1;
        std::cout << "New client connected with unique ID: " << uniqueClientID << std::endl;

        // Launch a dedicated handler thread for this client.
        std::thread(clientHandler, client_socket, uniqueClientID).detach();
    }

    // Cleanup code (if ever reached).
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
