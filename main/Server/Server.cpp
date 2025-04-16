#include <iostream>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <condition_variable>
#include <future>
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

std::atomic<bool> sessionOver(false);

struct ClientInfo {
    int clientSocket;
    bool ready;
};

// Thread-safe waiting clients for clients waiting to start a session.
std::mutex waitingMutex;
std::unordered_map<int, ClientInfo> waitingClients;

// A structure to hold pairing information.
struct PairInfo {
    int partnerID;
    int partnerSocket;
};

// Global pairing map (clientID -> PairInfo) used to pass pairing info to waiting clients.
std::unordered_map<int, PairInfo> pairingMap;

// Condition variable to signal a waiting client when it has been paired.
std::condition_variable waitingCV;

//----------------------------------------------------------------------------
// Helper: Retrieve the local IP address.
std::string getLocalIP() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        return "";
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);  // any available port
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);  // Google's DNS
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

//----------------------------------------------------------------------------
// Broadcast the server IP address for clients to discover.
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
        int sent_bytes1 = sendto(udp_socket, message.c_str(), message.size(), 0,
            reinterpret_cast<struct sockaddr*>(&broadcast_addr1),
            sizeof(broadcast_addr1));
        if (sent_bytes1 < 0)
            std::cerr << "Broadcast to 192.168.7.255 failed!" << std::endl;
        else
            std::cout << "Broadcasting to 192.168.7.255: " << message << std::endl;
        int sent_bytes2 = sendto(udp_socket, message.c_str(), message.size(), 0,
            reinterpret_cast<struct sockaddr*>(&broadcast_addr2),
            sizeof(broadcast_addr2));
        if (sent_bytes2 < 0)
            std::cerr << "Broadcast to 255.255.255.255 failed!" << std::endl;
        else
            std::cout << "Broadcasting to 255.255.255.255: " << message << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
    }
#ifdef _WIN32
    closesocket(udp_socket);
#else
    close(udp_socket);
#endif
}

//----------------------------------------------------------------------------
// For monitoring waiting clients (for debugging).
void displayWaitingClients() {
    std::lock_guard<std::mutex> lock(waitingMutex);
    if (waitingClients.empty())
        std::cout << "No clients waiting." << std::endl;
    else {
        std::cout << "Clients waiting: ";
        for (const auto& entry : waitingClients)
            std::cout << "ClientID " << entry.first << " (socket=" << entry.second.clientSocket
            << ", ready=" << (entry.second.ready ? "true" : "false") << ") ";
        std::cout << std::endl;
    }
}

void waitingClientsMonitor() {
    while (true) {
        displayWaitingClients();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

//----------------------------------------------------------------------------
// Timer thread function for a session.
void timerThread(int clientSocket1, int clientSocket2, bool& sessionActive) {
    Timer sessionTimer(10);  // e.g., 30 seconds session timer
    while (!sessionTimer.isTimeUp()) {
        std::string currentTimeStr = sessionTimer.UpdateTime();
        json j;
        j["type"] = "TIME_UPDATE";
        j["time"] = currentTimeStr;
        std::string message = j.dump();
        send(clientSocket1, message.c_str(), message.size(), 0);
        send(clientSocket2, message.c_str(), message.size(), 0);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // When time is up, send GAME_OVER messages.
    if (score1 > score2) {
        json j1; j1["type"] = "GAME_OVER"; j1["message"] = "Time's up! You Won! "; j1["score"] = score1; j1["outcome"] = "win!";
        std::string gameOverMsg1 = j1.dump();
        send(clientSocket1, gameOverMsg1.c_str(), gameOverMsg1.size(), 0);
        json j2; j2["type"] = "GAME_OVER"; j2["message"] = "Time's up! You Lost! "; j2["score"] = score2; j2["outcome"] = "lose!";
        std::string gameOverMsg2 = j2.dump();
        send(clientSocket2, gameOverMsg2.c_str(), gameOverMsg2.size(), 0);
    }
    else if (score2 > score1) {
        json j1; j1["type"] = "GAME_OVER"; j1["message"] = "Time's up! You Lost! "; j1["score"] = score1; j1["outcome"] = "lose!";
        std::string gameOverMsg1 = j1.dump();
        send(clientSocket1, gameOverMsg1.c_str(), gameOverMsg1.size(), 0);
        json j2; j2["type"] = "GAME_OVER"; j2["message"] = "Time's up! You Won! "; j2["score"] = score2; j2["outcome"] = "win!";
        std::string gameOverMsg2 = j2.dump();
        send(clientSocket2, gameOverMsg2.c_str(), gameOverMsg2.size(), 0);
    }
    else {
        json j1; j1["type"] = "GAME_OVER"; j1["message"] = "Time's up! It's a draw! "; j1["score"] = score1; j1["outcome"] = "draw!";
        std::string gameOverMsg1 = j1.dump();
        send(clientSocket1, gameOverMsg1.c_str(), gameOverMsg1.size(), 0);
        json j2; j2["type"] = "GAME_OVER"; j2["message"] = "Time's up! It's a draw! "; j2["score"] = score2; j2["outcome"] = "draw!";
        std::string gameOverMsg2 = j2.dump();
        send(clientSocket2, gameOverMsg2.c_str(), gameOverMsg2.size(), 0);
    }
    json sessionEndMsg;
    sessionEndMsg["type"] = "SESSION_OVER";
    sessionEndMsg["message"] = "Session ended. Press START_GAME to play again.";
    std::string endMsgStr = sessionEndMsg.dump();
    send(clientSocket1, endMsgStr.c_str(), endMsgStr.size(), 0);
    send(clientSocket2, endMsgStr.c_str(), endMsgStr.size(), 0);
    sessionActive = false;
}

//----------------------------------------------------------------------------
// Session handler now does not return until the session is ended.
// It remains identical except that it does not attempt any synchronization itself.
void sessionHandler(int clientSocket1, int clientID1, int clientSocket2, int clientID2, std::atomic<bool>& sessionOver)
{   
    sessionOver.store(false);
    bool sessionActive = true;
    // Notify both clients that the session is starting.
    std::string sessionMessage1 = "You are now in a session with client " + std::to_string(clientID2) + "\n";
    std::string sessionMessage2 = "You are now in a session with client " + std::to_string(clientID1) + "\n";
    send(clientSocket1, sessionMessage1.c_str(), sessionMessage1.size(), 0);
    send(clientSocket2, sessionMessage2.c_str(), sessionMessage2.size(), 0);
    std::cout << "Session started between client " << clientID1 << " and client " << clientID2 << std::endl;

    // Assign random shapes.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, static_cast<int>(ShapeType::Count) - 1);
    json shapeMsg1, shapeMsg2, shapeMsg3;
    shapeMsg1["type"] = "SHAPE_ASSIGN";
    shapeMsg1["shapeType"] = dist(gen);
    shapeMsg2["type"] = "SHAPE_ASSIGN";
    shapeMsg2["shapeType"] = dist(gen);
    shapeMsg3["type"] = "SHAPE_ASSIGN";
    shapeMsg3["shapeType"] = dist(gen);
    std::string shape1Str = shapeMsg1.dump() + "\n";
    std::string shape2Str = shapeMsg2.dump() + "\n";
    std::string shape3Str = shapeMsg3.dump() + "\n";
    send(clientSocket1, shape1Str.c_str(), shape1Str.size(), 0);
    send(clientSocket2, shape1Str.c_str(), shape1Str.size(), 0);
    send(clientSocket1, shape2Str.c_str(), shape2Str.size(), 0);
    send(clientSocket2, shape2Str.c_str(), shape2Str.size(), 0);
    send(clientSocket1, shape3Str.c_str(), shape3Str.size(), 0);
    send(clientSocket2, shape3Str.c_str(), shape3Str.size(), 0);

    // Launch timer thread.
    std::thread t(timerThread, clientSocket1, clientSocket2, std::ref(sessionActive));
    t.detach();

    char buffer[1024];
    while (sessionActive) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket1, &readfds);
        FD_SET(clientSocket2, &readfds);
        int maxfd = std::max(clientSocket1, clientSocket2);
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
        // Process messages from client 1.
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
                        send(clientSocket1, msg.c_str(), msg.size(), 0);
                        send(clientSocket2, msg.c_str(), msg.size(), 0);
                    }
                    else if (j["type"] == "SCORE_REQUEST") {
                        std::vector<int> rows = j["rows"];
                        std::vector<int> cols = j["columns"];
                        int totalCleared = static_cast<int>(rows.size() + cols.size());
                        double multiplier = (totalCleared > 1) ? (1.0 + 0.5 * (totalCleared - 1)) : 1.0;
                        int earnedScore = static_cast<int>(totalCleared * 100 * multiplier);
                        if (earnedScore > 400)
                            score1 += 400;
                        else
                            score1 += earnedScore;
                        json response;
                        response["type"] = "SCORE_RESPONSE";
                        response["score"] = score1;
                        std::string responseStr = response.dump();
                        send(clientSocket1, responseStr.c_str(), responseStr.size(), 0);
                        send(clientSocket2, responseStr.c_str(), responseStr.size(), 0);
                        json updateMsg;
                        updateMsg["type"] = "SCORE_UPDATE";
                        updateMsg["opponentScore"] = score1;
                        std::string updateStr = updateMsg.dump();
                        send(clientSocket2, updateStr.c_str(), updateStr.size(), 0);
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
        // Process messages from client 2.
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
                        send(clientSocket1, msg.c_str(), msg.size(), 0);
                    }
                    else if (j["type"] == "SCORE_REQUEST") {
                        std::vector<int> rows = j["rows"];
                        std::vector<int> cols = j["columns"];
                        int totalCleared = static_cast<int>(rows.size() + cols.size());
                        double multiplier = (totalCleared > 1) ? (1.0 + 0.5 * (totalCleared - 1)) : 1.0;
                        int earnedScore = static_cast<int>(totalCleared * 100 * multiplier);
                        if (earnedScore > 400)
                            score2 += 400;
                        else
                            score2 += earnedScore;
                        json response;
                        response["type"] = "SCORE_RESPONSE";
                        response["score"] = score2;
                        std::string responseStr = response.dump();
                        send(clientSocket2, responseStr.c_str(), responseStr.size(), 0);
                        send(clientSocket1, responseStr.c_str(), responseStr.size(), 0);
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
    // Cleanup waiting state.
    {
        std::lock_guard<std::mutex> lock(waitingMutex);
        waitingClients.erase(clientID1);
        waitingClients.erase(clientID2);
    }
    std::cout << "Session between client " << clientID1 << " and client " << clientID2 << " ended." << std::endl;
    score1 = 0;
    score2 = 0;

    json sessionEndMsg;
    sessionEndMsg["type"] = "SESSION_OVER";
    sessionEndMsg["message"] = "Session ended. Press START_GAME to play again.";
    std::string endMsgStr = sessionEndMsg.dump();
    send(clientSocket1, endMsgStr.c_str(), endMsgStr.size(), 0);
    send(clientSocket2, endMsgStr.c_str(), endMsgStr.size(), 0);

    // Signal that the session has ended.
    sessionOver.store(true);
}

//----------------------------------------------------------------------------
// Updated clientHandler now runs an outer loop so that after each session the thread resumes listening for a new START_GAME.
void clientHandler(int client_socket, int clientID) {
    char buffer[256];
    while (true) {
        // Clear any stale waiting state.
        {
            std::lock_guard<std::mutex> lock(waitingMutex);
            waitingClients.erase(clientID);
        }
        // Wait for the client to send "START_GAME".
        while (true) {
            int bytesRead = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0) {
                std::cerr << "Client " << clientID << " disconnected or error occurred." << std::endl;
#ifdef _WIN32
                closesocket(client_socket);
#else
                close(client_socket);
#endif
                return;
            }
            buffer[bytesRead] = '\0';
            std::string message(buffer);
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
        // Pairing:
        int partnerSocket = -1;
        int partnerID = -1;
        bool paired = false;
        {
            std::lock_guard<std::mutex> lock(waitingMutex);
            waitingClients[clientID] = { client_socket, true };
            for (auto it = waitingClients.begin(); it != waitingClients.end(); ++it) {
                if (it->first != clientID && it->second.ready) {
                    partnerID = it->first;
                    partnerSocket = it->second.clientSocket;
                    waitingClients.erase(it);
                    waitingClients.erase(clientID);
                    pairingMap[partnerID] = { clientID, client_socket };
                    pairingMap[clientID] = { partnerID, partnerSocket };
                    paired = true;
                    waitingCV.notify_all();
                    std::cout << "Paired client " << partnerID << " with client " << clientID << std::endl;
                    break;
                }
            }
            if (!paired) {
                std::string waitMsg = "Waiting for another client to join...\n";
                send(client_socket, waitMsg.c_str(), waitMsg.size(), 0);
            }
        }
        if (!paired) {
            std::unique_lock<std::mutex> lock(waitingMutex);
            waitingCV.wait(lock, [&]() { return pairingMap.find(clientID) != pairingMap.end(); });
            PairInfo info = pairingMap[clientID];
            partnerID = info.partnerID;
            partnerSocket = info.partnerSocket;
            pairingMap.erase(clientID);
            std::cout << "Client " << clientID << " has been paired with client " << partnerID << std::endl;
        }
        // Notify both clients that the session is starting.
        json startMsgJson;
        startMsgJson["type"] = "START_GAME";
        std::string startMsg = startMsgJson.dump() + "\n";
        json tostartMsgJson;
        tostartMsgJson["type"] = "Tostart";
        tostartMsgJson["bool"] = true;
        std::string tostartMsg = tostartMsgJson.dump() + "\n";
        send(partnerSocket, startMsg.c_str(), startMsg.size(), 0);
        send(client_socket, startMsg.c_str(), startMsg.size(), 0);
        send(partnerSocket, tostartMsg.c_str(), tostartMsg.size(), 0);
        send(client_socket, tostartMsg.c_str(), tostartMsg.size(), 0);

        // Reset sessionOver flag for the new session.
        sessionOver.store(false);

        // Determine roles: the client with the lower ID will launch the session handler.
        if (clientID < partnerID) {
            std::cout << "Leader: Starting session for clients " << clientID << " and " << partnerID << std::endl;
            sessionHandler(client_socket, clientID, partnerSocket, partnerID, sessionOver);
        }
        else {
            // Instead of blocking with a future, wait until sessionOver is set.
            std::cout << "Partner: Waiting for leader to complete session for clients "
                << clientID << " and " << partnerID << std::endl;
            // Use a polling loop with a small sleep.
            while (!sessionOver.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        {
            std::lock_guard<std::mutex> lock(waitingMutex);
            waitingClients.erase(partnerID);
            waitingClients.erase(clientID);
            pairingMap.erase(partnerID);
            pairingMap.erase(clientID);
        }
        
        // At this point the session has ended.
        // The outer loop will continue so that the same connection listens for a new "START_GAME".
        std::cout << "Session between client " << clientID << " and client " << partnerID << " is complete. Ready for new game." << std::endl;
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
    // Start broadcaster thread.
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
    // Set up server TCP listener.
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
    // Main loop: accept new client connections.
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            std::cerr << "Connection failed" << std::endl;
            continue;
        }
        int uniqueClientID = clientIDCounter.fetch_add(1) + 1;
        std::cout << "New client connected with unique ID: " << uniqueClientID << std::endl;
        std::thread clientThread(clientHandler, client_socket, uniqueClientID);
        clientThread.detach();
        std::thread monitor(waitingClientsMonitor);
        monitor.detach();
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
