#include <iostream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <vector>
#include <mutex>
#include <unordered_map>  // Changed from <queue>
#include <utility>
#include <algorithm>  
#include <nlohmann/json.hpp>
#include "Timer.hpp"
#include <random>
#include "Shapes.hpp"

#define NOMINMAX
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#endif

#define PORT 1235             // Port for broadcasting and TCP listening
#define BROADCAST_INTERVAL 1  // Seconds between broadcasts

using json = nlohmann::json;
int score1 = 0;
int score2 = 0;

ShapeType shape;
json shapeMsg;

// Global atomic flag for broadcaster thread stop
std::atomic<bool> stopBroadcast(false);
// Global client ID counter
std::atomic<int> clientIDCounter(0);

// Thread-safe waiting clients for clients waiting to start a session.
// The unordered_map maps clientID to client_socket.
std::mutex waitingMutex;
std::unordered_map<int, int> waitingClients;  // Changed from std::queue

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

    sockaddr_in broadcast_addr1, broadcast_addr2;

    memset(&broadcast_addr1, 0, sizeof(broadcast_addr1));
    broadcast_addr1.sin_family = AF_INET;
    broadcast_addr1.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "192.168.7.255", &broadcast_addr1.sin_addr) <= 0) {
        std::cerr << "Invalid broadcast address 192.168.7.255" << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
        return;
    }

    memset(&broadcast_addr2, 0, sizeof(broadcast_addr2));
    broadcast_addr2.sin_family = AF_INET;
    broadcast_addr2.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "255.255.255.255", &broadcast_addr2.sin_addr) <= 0) {
        std::cerr << "Invalid broadcast address 255.255.255.255" << std::endl;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
        return;
    }

    std::string message = "SERVER_IP:" + ip + ":" + std::to_string(PORT);
    while (!stopBroadcast.load()) {
        // Broadcast to 192.168.7.255
        int sent_bytes1 = sendto(udp_socket, message.c_str(), message.size(), 0,
            reinterpret_cast<struct sockaddr*>(&broadcast_addr1),
            sizeof(broadcast_addr1));
        if (sent_bytes1 < 0) {
            std::cerr << "Broadcast to 192.168.7.255 failed!" << std::endl;
        }
        else {
            std::cout << "Broadcasting to 192.168.7.255: " << message << std::endl;
        }

        // Broadcast to 255.255.255.255
        int sent_bytes2 = sendto(udp_socket, message.c_str(), message.size(), 0,
            reinterpret_cast<struct sockaddr*>(&broadcast_addr2),
            sizeof(broadcast_addr2));
        if (sent_bytes2 < 0) {
            std::cerr << "Broadcast to 255.255.255.255 failed!" << std::endl;
        }
        else {
            std::cout << "Broadcasting to 255.255.255.255: " << message << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
    }

#ifdef _WIN32
    closesocket(udp_socket);
#else
    close(udp_socket);
#endif
}

// Timer thread function for a session
void timerThread(int clientSocket1, int clientSocket2) {
    Timer sessionTimer(50);  // Create a timer instance for 3 minutes

    while (!sessionTimer.isTimeUp()) {
        std::string currentTimeStr = sessionTimer.UpdateTime();

        // Package the time string in JSON so clients can handle it appropriately.
        json j;
        j["type"] = "TIME_UPDATE";
        j["time"] = currentTimeStr;
        std::string message = j.dump();

        // Send the timer update to both clients.
        send(clientSocket1, message.c_str(), message.size(), 0);
        send(clientSocket2, message.c_str(), message.size(), 0);

        std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait approximately 1 second
    }

    // Optionally notify clients that time is up.
    // For Client 1:

    if (score1 > score2){
        json j1;
        j1["type"] = "GAME_OVER";
        j1["message"] = "Time's up! The game is over! You Won! ";
        j1["score"] = score1;
		j1["outcome"] = "win!";
        std::string gameOverMsg1 = j1.dump();
        send(clientSocket1, gameOverMsg1.c_str(), gameOverMsg1.size(), 0);

        json j2;
        j2["type"] = "GAME_OVER";
        j2["message"] = "Time's up! The game is over! You Lost! ";
		j2["outcome"] = "lose!";
        j2["score"] = score2;
        std::string gameOverMsg2 = j2.dump();
        send(clientSocket2, gameOverMsg2.c_str(), gameOverMsg2.size(), 0);
    }
    else if (score2 > score1) {
        json j1;
        j1["type"] = "GAME_OVER";
        j1["message"] = "Time's up! The game is over! You Lost! ";
        j1["score"] = score1;
        j1["outcome"] = "lose!";
        std::string gameOverMsg1 = j1.dump();
        send(clientSocket1, gameOverMsg1.c_str(), gameOverMsg1.size(), 0);

        // For Client 2:
        json j2;
        j2["type"] = "GAME_OVER";
        j2["message"] = "Time's up! The game is over! You Won! ";
        j2["score"] = score2;
        j2["outcome"] = "win!";
        std::string gameOverMsg2 = j2.dump();
        send(clientSocket2, gameOverMsg2.c_str(), gameOverMsg2.size(), 0);
    }
    else if (score1 == score2) {
        json j1;
        j1["type"] = "GAME_OVER";
        j1["message"] = "Time's up! The game is over! Its a draw! ";
        j1["score"] = score1;
        j1["outcome"] = "draw!";
        std::string gameOverMsg1 = j1.dump();
        send(clientSocket1, gameOverMsg1.c_str(), gameOverMsg1.size(), 0);

        // For Client 2:
        json j2;
        j2["type"] = "GAME_OVER";
        j2["message"] = "Time's up! The game is over! Its a draw! ";
        j2["score"] = score2;
        j2["outcome"] = "draw!";
        std::string gameOverMsg2 = j2.dump();
        send(clientSocket2, gameOverMsg2.c_str(), gameOverMsg2.size(), 0);
    }
    
}

// Session handler: handles communication between two paired clients.
void sessionHandler(int clientSocket1, int clientID1, int clientSocket2, int clientID2) {
    // Notify both clients of session start.
    std::string sessionMessage1 = "You are now in a session with client " + std::to_string(clientID2) + "\n";
    std::string sessionMessage2 = "You are now in a session with client " + std::to_string(clientID1) + "\n";
    send(clientSocket1, sessionMessage1.c_str(), sessionMessage1.size(), 0);
    send(clientSocket2, sessionMessage2.c_str(), sessionMessage2.size(), 0);
    std::cout << "Session started between client " << clientID1 << " and client " << clientID2 << std::endl;

    // 2. Assign random shapes
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, static_cast<int>(ShapeType::Count) - 1);

    // Assign shapes to clients.
    ShapeType shape1 = static_cast<ShapeType>(dist(gen));
    ShapeType shape2 = static_cast<ShapeType>(dist(gen));
    ShapeType shape3 = static_cast<ShapeType>(dist(gen));
    json shapeMsg1;
    json shapeMsg2;
    json shapeMsg3;
    shapeMsg1["type"] = "SHAPE_ASSIGN";
    shapeMsg1["shapeType"] = static_cast<int>(shape1);
    shapeMsg2["type"] = "SHAPE_ASSIGN";
    shapeMsg2["shapeType"] = static_cast<int>(shape2);
    shapeMsg3["type"] = "SHAPE_ASSIGN";
    shapeMsg3["shapeType"] = static_cast<int>(shape3);
    send(clientSocket1, shapeMsg1.dump().c_str(), shapeMsg1.dump().size(), 0);
    send(clientSocket2, shapeMsg1.dump().c_str(), shapeMsg1.dump().size(), 0);
    send(clientSocket1, shapeMsg2.dump().c_str(), shapeMsg2.dump().size(), 0);
    send(clientSocket2, shapeMsg2.dump().c_str(), shapeMsg2.dump().size(), 0);
    send(clientSocket1, shapeMsg3.dump().c_str(), shapeMsg3.dump().size(), 0);
    send(clientSocket2, shapeMsg3.dump().c_str(), shapeMsg3.dump().size(), 0);

    // Launch the timer thread for this specific session.
    std::thread t(timerThread, clientSocket1, clientSocket2);
    t.detach();

    char buffer[1024];
    bool sessionActive = true;

    while (sessionActive) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket1, &readfds);
        FD_SET(clientSocket2, &readfds);
        int maxfd = std::max(clientSocket1, clientSocket2);

        // Set timeout (optional, e.g., 1 second)
        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (activity < 0) {
            std::cerr << "select() error." << std::endl;
            break;
        }

        if (activity == 0)
            continue;

        // Process socket 1 if there is activity.
        if (FD_ISSET(clientSocket1, &readfds)) {
            int bytesRead = recv(clientSocket1, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string msg(buffer);
                try {
                    json j = json::parse(msg);
                    if (j.contains("type") && j["type"] == "DRAG_UPDATE") {
                        std::cout << "Received DRAG_UPDATE from client " << clientID1 << ":\n";

                        shape = static_cast<ShapeType>(dist(gen));
                        shapeMsg["type"] = "SHAPE_ASSIGN";
                        shapeMsg["shapeType"] = static_cast<int>(shape);

                        send(clientSocket1, shapeMsg.dump().c_str(), shapeMsg.dump().size(), 0);
                        send(clientSocket2, shapeMsg.dump().c_str(), shapeMsg.dump().size(), 0);

                        if (j.contains("blocks")) {
                            for (const auto& block : j["blocks"]) {
                                int x = block.value("x", -1);
                                int y = block.value("y", -1);
                                std::cout << "   Block: (" << x << ", " << y << ")\n";
                            }
                        }
                        // Forward the update to both clients.
                        send(clientSocket1, msg.c_str(), msg.size(), 0);
                        send(clientSocket2, msg.c_str(), msg.size(), 0);
                    }
                    else if (j["type"] == "SCORE_REQUEST") {
                        std::cout << "[Server] Received SCORE_REQUEST message from client " << clientID1 << ": " << j.dump() << "\n";

                        std::vector<int> rows = j["rows"];
                        std::vector<int> cols = j["columns"];

                        int totalCleared = static_cast<int>(rows.size() + cols.size());
                        double multiplier = (totalCleared > 1) ? (1.0 + 0.5 * (totalCleared - 1)) : 1.0;
                        int earnedScore = static_cast<int>(totalCleared * 100 * multiplier);
                        std::cout << "[Server] Total Cleared is  " << totalCleared << "\n";
                        std::cout << "[Server] The multiplier is " << multiplier << "\n";
                        std::cout << "[Server] Total earned score: " << earnedScore << "\n";
                        if (earnedScore > 400) {
                            score1 += 400;
                        }
                        else {
                            std::cout << score2;
                            std::cout << earnedScore;
                            score1 += earnedScore;
                        }
                        std::cout << "[Server] The score of Client " << clientID1 << " Is " << score1 << "\n";
                        std::cout << "[Server] Client " << clientID1 << " cleared "
                            << rows.size() << " rows and "
                            << cols.size() << " cols => Score: " << score1 << "\n";

                        json response;
                        response["type"] = "SCORE_RESPONSE";
                        response["score"] = score1;
                        std::string responseStr = response.dump();
                        std::cout << "Sending score to client";

                        std::cout << "[Server] Sending SCORE_RESPONSE: " << responseStr << "\n";
                        std::cout << "[Debug] Sending on socket: " << clientSocket1 << "\n";
                        int sent = send(clientSocket1, responseStr.c_str(), responseStr.size(), 0);
                        std::cout << "[Server] send() returned: " << sent << "\n";

                        //Send Score to Opponent
                        json updateMsg;
                        updateMsg["type"] = "SCORE_UPDATE";
                        updateMsg["opponentScore"] = score1;

                        std::string updateStr = updateMsg.dump();
                        send(clientSocket2, updateStr.c_str(), updateStr.size(), 0);

                        if (sent <= 0) {
                            std::cerr << "[Server] Failed to send SCORE_RESPONSE to client " << clientID1 << "\n";
                        }
                    }

                }
                catch (...) {
                    std::string forwardMsg = "Client " + std::to_string(clientID1) + ": " + msg;
                    send(clientSocket1, forwardMsg.c_str(), forwardMsg.size(), 0);
                }
            }
            else if (bytesRead == 0) {
                std::cout << "Client " << clientID1 << " disconnected." << std::endl;
                sessionActive = false;
            }
            else {
                std::cerr << "Error reading from client " << clientID1 << std::endl;
                sessionActive = false;
            }
        }

        // Process socket 2 if there is activity.
        if (FD_ISSET(clientSocket2, &readfds)) {
            int bytesRead = recv(clientSocket2, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string msg(buffer);
                try {
                    json j = json::parse(msg);
                    if (j.contains("type") && j["type"] == "DRAG_UPDATE") {
                        std::cout << "Received DRAG_UPDATE from client " << clientID2 << ":\n";
                        shape = static_cast<ShapeType>(dist(gen));
                        shapeMsg["type"] = "SHAPE_ASSIGN";
                        shapeMsg["shapeType"] = static_cast<int>(shape);

                        send(clientSocket1, shapeMsg.dump().c_str(), shapeMsg.dump().size(), 0);
                        send(clientSocket2, shapeMsg.dump().c_str(), shapeMsg.dump().size(), 0);

                        if (j.contains("blocks")) {
                            for (const auto& block : j["blocks"]) {
                                int x = block.value("x", -1);
                                int y = block.value("y", -1);
                                std::cout << "   Block: (" << x << ", " << y << ")\n";
                            }
                        }
                        // Forward the update to client 1.
                        send(clientSocket1, msg.c_str(), msg.size(), 0);
                    }
                    else if (j["type"] == "SCORE_REQUEST") {
                        std::vector<int> rows = j["rows"];
                        std::vector<int> cols = j["columns"];

                        int totalCleared = static_cast<int>(rows.size() + cols.size());
                        double multiplier = (totalCleared > 1) ? (1.0 + 0.5 * (totalCleared - 1)) : 1.0;
                        int earnedScore = static_cast<int>(totalCleared * 100 * multiplier);
                        if (earnedScore > 400) {
                            score2 += 400;
                        }
                        else {
                            std::cout << score2;
                            std::cout << earnedScore;
                            score2 += earnedScore;
                        }
                        std::cout << score2;
                        std::cout << "Client " << clientID2 << " cleared "
                            << rows.size() << " rows and "
                            << cols.size() << " cols => Score: " << score2 << "\n";

                        json response;
                        response["type"] = "SCORE_RESPONSE";
                        response["score"] = score2;
                        std::cout << "Sending score to client";
                        std::string responseStr = response.dump();

                        std::cout << "[Server] Sending SCORE_RESPONSE: " << responseStr << "\n";
                        std::cout << "[Debug] Sending on socket: " << clientSocket1 << "\n";
                        int sent = send(clientSocket1, responseStr.c_str(), responseStr.size(), 0);
                        std::cout << "[Server] send() returned: " << sent << "\n";

                        send(clientSocket2, responseStr.c_str(), responseStr.size(), 0);

                        //Send Score to Opponent
                        json updateMsg;
                        updateMsg["type"] = "SCORE_UPDATE";
                        updateMsg["opponentScore"] = score2;

                        std::string updateStr = updateMsg.dump();
                        send(clientSocket1, updateStr.c_str(), updateStr.size(), 0);
                    }

                }
                catch (...) {
                    std::string forwardMsg = "Client " + std::to_string(clientID2) + ": " + msg;
                    send(clientSocket2, forwardMsg.c_str(), forwardMsg.size(), 0);
                }
            }
            else if (bytesRead == 0) {
                std::cout << "Client " << clientID2 << " disconnected." << std::endl;
                sessionActive = false;
            }
            else {
                std::cerr << "Error reading from client " << clientID2 << std::endl;
                sessionActive = false;
            }
        }
    }

    // Clean up sockets when session ends.
#ifdef _WIN32
    closesocket(clientSocket1);
    closesocket(clientSocket2);
#else
    close(clientSocket1);
    close(clientSocket2);
#endif
    std::cout << "Session between client " << clientID1 << " and client " << clientID2 << " ended." << std::endl;
    score1 = 0;
    score2 = 0;
}

// Dedicated client handler: continuously listens for messages from the client.
void clientHandler(int client_socket, int clientID) {
    char buffer[256];

    // First, wait for the client to send the "START_GAME" command.
    while (true) {
        int bytesRead = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            std::cerr << "Client " << clientID << " disconnected or an error occurred." << std::endl;
            // Remove from waitingClients (if in it).
            {
                std::lock_guard<std::mutex> lock(waitingMutex);
                auto it = waitingClients.find(clientID);
                if (it != waitingClients.end()) {
                    waitingClients.erase(it);
                    std::cout << "Client " << clientID << " was removed from waitingClients due to disconnection." << std::endl;
                }
            }
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
            return;
        }
        buffer[bytesRead] = '\0';
        std::string message(buffer);
        // Remove newline characters
        message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
        message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());

        if (message == "START_GAME") {
            std::cout << "Client " << clientID << " requested to start a game." << std::endl;
            break;
        }
        else {
            std::cerr << "[Server] Unknown message from client " << clientID << ": " << message << "\n";
        }
    }

    // Now the client has requested to start a game.
    {
        std::lock_guard<std::mutex> lock(waitingMutex);
        if (!waitingClients.empty()) {
            // Pair with an arbitrary waiting client.
            auto it = waitingClients.begin();
            int otherID = it->first;
            int otherSocket = it->second;
            waitingClients.erase(it);  // Remove the waiting client.

            // Notify both clients about game start.
            std::string startMsg = "START_GAME";
            send(otherSocket, startMsg.c_str(), startMsg.size(), 0);
            send(client_socket, startMsg.c_str(), startMsg.size(), 0);

            // Launch a new session thread for these two clients.
            std::thread sessionThread(sessionHandler, otherSocket, otherID, client_socket, clientID);
            sessionThread.detach();
#ifdef _WIN32
            HANDLE threadHandle = static_cast<HANDLE>(sessionThread.native_handle());
            SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);
#else
            sched_param sch_params;
            sch_params.sched_priority = 80;
            if (pthread_setschedparam(sessionThread.native_handle(), SCHED_FIFO, &sch_params)) {
                std::cerr << "Failed to set thread scheduling: insufficient privileges?" << std::endl;
            }
#endif

            json StartMsg;
            StartMsg["type"] = "Tostart";
            StartMsg["bool"] = true;
            send(otherSocket, StartMsg.dump().c_str(), StartMsg.dump().size(), 0);
            send(client_socket, StartMsg.dump().c_str(), StartMsg.dump().size(), 0);

            std::cout << "Paired client " << otherID << " with client " << clientID << std::endl;

            // Return here; sessionThread now takes over communication on both sockets.
            return;
        }
        else {
            // No client is waiting; add this client to waitingClients.
            waitingClients.emplace(clientID, client_socket);
            std::string waitMsg = "Waiting for another client to join...\n";
            send(client_socket, waitMsg.c_str(), waitMsg.size(), 0);
        }
    }

    // --- Start a loop that monitors the waiting client socket.
    // This loop periodically checks if the waiting client has disconnected.
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds);
        timeval tv;
        tv.tv_sec = 1;  // 1 second timeout
        tv.tv_usec = 0;

        int activity = select(client_socket + 1, &readfds, nullptr, nullptr, &tv);
        if (activity < 0) {
            std::cerr << "select() error on waiting client " << clientID << std::endl;
            break;  // Exit on error.
        }
        else if (activity == 0) {
            // Timeout occurred; check if this client is still in waitingClients.
            std::lock_guard<std::mutex> lock(waitingMutex);
            if (waitingClients.find(clientID) == waitingClients.end()) {
                // The client was paired and removed from waitingClients.
                break;
            }
            // Otherwise, simply continue waiting.
            continue;
        }

        if (FD_ISSET(client_socket, &readfds)) {
            // There is data available.
            int br = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (br <= 0) {
                // Error or disconnect detected.
                std::lock_guard<std::mutex> lock(waitingMutex);
                waitingClients.erase(clientID);
                std::cerr << "Waiting client " << clientID << " disconnected." << std::endl;
#ifdef _WIN32
                closesocket(client_socket);
#else
                close(client_socket);
#endif
                return;
            }
            else {
                // Optionally process any unexpected messages while waiting.
                buffer[br] = '\0';
                std::string extraMsg(buffer);
                std::cout << "Waiting client " << clientID << " sent an extra message: " << extraMsg << std::endl;
            }
        }
    }

    // If the loop exits, it means the client was paired (waitingClients entry was removed).
    // At this point, the sessionThread has been started for this client.
    std::cout << "Client " << clientID << " has been paired. Ending waiting loop." << std::endl;
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

    // Start the broadcaster thread for client discovery.
    std::thread broadcaster(broadcastIP, localIP);
#ifdef _WIN32
    HANDLE broadcastHandle = static_cast<HANDLE>(broadcaster.native_handle());
    SetThreadPriority(broadcastHandle, THREAD_PRIORITY_LOWEST);
#else
    sched_param bro_params;
    bro_params.sched_priority = 10;
    if (pthread_setschedparam(broadcaster.native_handle(), SCHED_OTHER, &bro_params)) {
        std::cerr << "Failed to lower broadcaster thread scheduling." << std::endl;
    }
#endif
    broadcaster.detach();

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

        // Launch a dedicated thread for this client.
        std::thread clientThread(clientHandler, client_socket, uniqueClientID);
        clientThread.detach();
    }

    // Cleanup code (if reached).
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
