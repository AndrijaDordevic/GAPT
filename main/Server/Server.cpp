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
#include <signal.h>
#include <openssl/hmac.h>
#include <iomanip>


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
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>
#endif

#define PORT 1235             // Port for broadcasting and TCP listening
#define BROADCAST_INTERVAL 1  // Seconds between broadcasts

using json = nlohmann::json;
int score1 = 0;
int score2 = 0;


uint64_t id = 0;

ShapeType shape;
json shapeMsg;

// Global atomic flag for broadcaster thread stop
std::atomic<bool> stopBroadcast(false);
// Global client ID counter
std::atomic<int> clientIDCounter(0);

std::atomic<bool> sessionOver(false);

// Shared HMAC secret
static const std::string SHARED_SECRET = "my?very?strong?key";

struct ClientInfo {
    int clientSocket;
    bool ready;
    uint64_t lastMessageId = 0;
};

std::unordered_map<int, ClientInfo> clientStates; // clientID -> state
std::mutex clientStatesMutex;
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

void ignoreSigpipe() {
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
}

// Computes HMAC-SHA256(secret, data) and returns hex string.
static std::string computeHMAC(const std::string& data, const std::string& secret) {
    unsigned char* result;
    unsigned int len = EVP_MAX_MD_SIZE;

    result = HMAC(
        EVP_sha256(),
        reinterpret_cast<const unsigned char*>(secret.data()), secret.size(),
        reinterpret_cast<const unsigned char*>(data.data()), data.size(),
        nullptr, nullptr
    );
    // hex-encode
    std::ostringstream hex;
    for (unsigned int i = 0; i < 32; ++i)  // SHA256 is 32 bytes
        hex << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    return hex.str();
}

// Constant-time comparison to avoid timing attacks:
static bool hmacEquals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i)
        diff |= a[i] ^ b[i];
    return diff == 0;
}

//------------------------------------------------------------------------------
// Safe send wrapper (prevents SIGPIPE)
int safeSend(int sock, const char* buf, size_t len) {
#ifdef _WIN32
    return send(sock, buf, (int)len, 0);
#else
    return send(sock, buf, len, MSG_NOSIGNAL);
#endif
}

bool sendSecure(int sock, json j, const std::string& secret) {
    // 1) Serialize without HMAC
    std::string body = j.dump();
    // 2) Compute HMAC
    std::string tag = computeHMAC(body, secret);
    // 3) Add it
    j["hmac"] = tag;
    std::string withTag = j.dump();
    // 4) Send (add newline if your protocol expects it)
    withTag += "\n";
    return safeSend(sock, withTag.c_str(), withTag.size()) >= 0;
}

// Add this function to read and verify secure messages
bool recvSecure(int sock, json& j, const std::string& secret, int clientID) {
    char buffer[4096];
    int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        return false;
    }
    buffer[bytesRead] = '\0';

    try {
        j = json::parse(buffer);
        std::cout << "Received JSON from client " << clientID << ": " << j.dump() << "\n";

        // Verify HMAC first
        if (j.contains("hmac")) {
            std::string receivedTag = j["hmac"];
            json body = j;
            body.erase("hmac");
            std::string bodyStr = body.dump();

            if (!hmacEquals(receivedTag, computeHMAC(bodyStr, secret))) {
                std::cerr << "HMAC verification failed\n";
                return false;
            }

            // Extract message ID from type field (e.g. "123SCORE_REQUEST")
            if (j.contains("type") && j["type"].is_string()) {
                std::string typeStr = j["type"].get<std::string>();

                // Find where the ID ends and type begins
                size_t typeStart = typeStr.find_first_not_of("0123456789");
                if (typeStart != std::string::npos && typeStart > 0) {
                    uint64_t currentId = std::stoull(typeStr.substr(0, typeStart));

                    // Per-client replay protection
                    {
                        std::lock_guard<std::mutex> lock(clientStatesMutex);
                        auto& clientState = clientStates[clientID];

                        std::cout << "Client " << clientID << " - Current ID: " << currentId
                            << ", Last ID: " << clientState.lastMessageId << "\n";

                        if (currentId <= clientState.lastMessageId) {
                            std::cerr << "Replay attack detected from client " << clientID
                                << "! Message ID " << currentId
                                << " <= last seen " << clientState.lastMessageId << "\n";
                            return false;
                        }
                        clientState.lastMessageId = currentId;
                    }

                    // Remove ID from type for normal processing
                    j["type"] = typeStr.substr(typeStart);
                }
            }

            j.erase("hmac");
            return true;
        }
        return false;
    }
    catch (...) {
        std::cerr << "Failed to parse JSON message\n";
        return false;
    }
}

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

bool isSocketAlive(int sock) {
    // prepare fd_sets
#ifdef _WIN32
    fd_set readfds, exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&exceptfds);
    FD_SET(static_cast<SOCKET>(sock), &readfds);
    FD_SET(static_cast<SOCKET>(sock), &exceptfds);
    // Winsock select uses nfds ignored
    timeval tv{ 0, 0 };
    int ret = select(0, &readfds, nullptr, &exceptfds, &tv);
#else
    fd_set readfds, exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&exceptfds);
    FD_SET(sock, &readfds);
    FD_SET(sock, &exceptfds);
    timeval tv{ 0, 0 };
    int ret = select(sock + 1, &readfds, nullptr, &exceptfds, &tv);
#endif

    if (ret < 0) {
        // select() failed — assume dead
        return false;
    }
    if (FD_ISSET(sock, &exceptfds)) {
        // socket in exception state => dead
        return false;
    }
    if (ret == 0) {
        // no events => socket still up
        return true;
    }

    // if we get here, there's something to read (or it's hung up)
    if (FD_ISSET(sock, &readfds)) {
        // peek one byte
#ifdef _WIN32
        int n = recv(static_cast<SOCKET>(sock), nullptr, 0, MSG_PEEK);
#else
        char buf;
        int n = recv(sock, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
#endif
        if (n <= 0) {
            // 0 = orderly shutdown, <0 = error
            return false;
        }
    }

    return true;
}

//----------------------------------------------------------------------------
// Timer thread function for a session.
void timerThread(int clientSocket1, int clientSocket2, std::shared_ptr<std::atomic<bool>> sessionActive)
{
   int remaining = 30;               // total seconds
    while (remaining > 0 && sessionActive->load()) {
        // format mm:ss
        int m = remaining / 60, s = remaining % 60;
        std::ostringstream ss;
        ss << std::setw(2) << std::setfill('0') << m
           << ":" << std::setw(2) << std::setfill('0') << s;

        json j = { {"type","TIME_UPDATE"}, {"time", ss.str()} };
        sendSecure(clientSocket1, j, SHARED_SECRET);
        sendSecure(clientSocket2, j, SHARED_SECRET);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        --remaining;
    }

    // If the session was aborted mid-timer:
    if (!sessionActive->load()) {
        // 1) Force timer to the remaining client
        json zeroed;
        zeroed["type"] = "TIME_UPDATE";
        zeroed["time"] = "00:00";
        if (isSocketAlive(clientSocket1))
            sendSecure(clientSocket1, zeroed, SHARED_SECRET);
        if (isSocketAlive(clientSocket2))
            sendSecure(clientSocket2, zeroed, SHARED_SECRET);

        // 2) Notify of session abort
        json abortMsg;
        abortMsg["type"] = "SESSION_ABORT";
        abortMsg["message"] = "Other player disconnected. Session aborted.";
        if (isSocketAlive(clientSocket1))
            sendSecure(clientSocket1, abortMsg, SHARED_SECRET);
        if (isSocketAlive(clientSocket2))
            sendSecure(clientSocket2, abortMsg, SHARED_SECRET);

        return;
    }

    if (score1 > score2) {
        json w1 = { {"type","GAME_OVER"},{"message","Time's up! You Won!"},{"score",score1},{"outcome","win!"} };
        json w2 = { {"type","GAME_OVER"},{"message","Time's up! You Lost!"},{"score",score2},{"outcome","lose!"} };
        sendSecure(clientSocket1, w1, SHARED_SECRET);
        sendSecure(clientSocket2, w2, SHARED_SECRET);
    }
    else if (score2 > score1) {
        json w1 = { {"type","GAME_OVER"},{"message","Time's up! You Lost!"},{"score",score1},{"outcome","lose!"} };
        json w2 = { {"type","GAME_OVER"},{"message","Time's up! You Won!"},{"score",score2},{"outcome","win!"} };
        sendSecure(clientSocket1, w1, SHARED_SECRET);
        sendSecure(clientSocket2, w2, SHARED_SECRET);
    }
    else {
        json d = { {"type","GAME_OVER"},{"message","Time's up! It's a draw!"},{"score",score1},{"outcome","draw!"} };
        sendSecure(clientSocket1, d, SHARED_SECRET);
        sendSecure(clientSocket2, d, SHARED_SECRET);
    }

    // Final SESSION_OVER prompt
    json endMsg = { {"type","SESSION_OVER"},
                   {"message","Session ended. Press START_GAME to play again."} };
    sendSecure(clientSocket1, endMsg, SHARED_SECRET);
    sendSecure(clientSocket2, endMsg, SHARED_SECRET);

    sessionActive->store(false);
}

//----------------------------------------------------------------------------
// Session handler now does not return until the session is ended.
void sessionHandler(int clientSocket1, int clientID1, int clientSocket2, int clientID2, std::atomic<bool>& sessionOver)
{
    sessionOver.store(false);
    auto sessionActive = std::make_shared<std::atomic<bool>>(true);

    json sessionMsg1;
    sessionMsg1["type"] = "SESSION_START";
    sessionMsg1["message"] = "You are now in a session with client " + std::to_string(clientID2);
    sendSecure(clientSocket1, sessionMsg1, SHARED_SECRET);

    json sessionMsg2;
    sessionMsg2["type"] = "SESSION_START";
    sessionMsg2["message"] = "You are now in a session with client " + std::to_string(clientID1);
    sendSecure(clientSocket2, sessionMsg2, SHARED_SECRET);

    std::cout << "Session started between client " << clientID1 << " and client " << clientID2 << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, static_cast<int>(ShapeType::Count) - 1);

    // Pre-generate a queue of shapes for this session
    const int totalShapes = 1000; // total number of predetermined shapes
    std::vector<ShapeType> shapeArray(totalShapes);
    for (int i = 0; i < totalShapes; ++i)
        shapeArray[i] = static_cast<ShapeType>(dist(gen));

    // 2) Two pointers
    size_t nextIdx1 = 0, nextIdx2 = 0;

    // Helper to build and send a SHAPE_ASSIGN
    auto sendShapeTo = [&](int sock, ShapeType s) {
        json j;
        j["type"] = "SHAPE_ASSIGN";
        j["shapeType"] = static_cast<int>(s);
        // sign+send
        if (!sendSecure(sock, j, SHARED_SECRET)) {
            std::cerr << "FAILED to send SHAPE_ASSIGN\n";
        }
    };

    // 3) Initial deal of 3 shapes each
    for (int i = 0; i < 3; ++i) {
        sendShapeTo(clientSocket1, shapeArray[nextIdx1++]);
        sendShapeTo(clientSocket2, shapeArray[nextIdx2++]);
    }


    std::thread t(timerThread, clientSocket1, clientSocket2, std::ref(sessionActive));
    t.detach();

    while (sessionActive->load()) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket1, &readfds);
        FD_SET(clientSocket2, &readfds);
        int maxfd = std::max(clientSocket1, clientSocket2);
        timeval tv{ 1, 0 }; // 1 second timeout

        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (activity < 0) {
            std::cerr << "select() error.\n";
            break;
        }
        if (activity == 0) continue;

        auto processClient = [&](int fromSocket, int toSocket, int& fromScore, int fromID, int toID) -> bool {
            json j;
            if (!recvSecure(fromSocket, j, SHARED_SECRET, fromID)) {
                std::cout << "Client " << fromID << " disconnected.\n";

                // Notify the other client immediately
                json sessionEndMsg;
                sessionEndMsg["type"] = "SESSION_OVER";
                sessionEndMsg["message"] = "Other player disconnected. Press START_GAME to play again.";
                sendSecure(toSocket, sessionEndMsg, SHARED_SECRET);

                sessionActive->store(false);
                return false; // break the loop
            }

            // Process the message (j is already parsed and verified)
            if (j.contains("type") && j["type"] == "DRAG_UPDATE") {
                std::cout << "Received DRAG_UPDATE from client " << fromID << ":\n";
                if (j.contains("blocks")) {
                    for (const auto& block : j["blocks"]) {
                        int x = block.value("x", -1);
                        int y = block.value("y", -1);
                        std::cout << "   Block: (" << x << ", " << y << ")\n";
                    }
                }

                // 1) Forward the drag-update to both (unchanged)
                sendSecure(clientSocket1,j, SHARED_SECRET);
                sendSecure(clientSocket2, j, SHARED_SECRET);

                // 2) Now pop _this_ client’s next shape and send _only_ to them
                if (fromID == clientID1) {
                    ShapeType next = (nextIdx1 < shapeArray.size())
                        ? shapeArray[nextIdx1++]
                        : static_cast<ShapeType>(dist(gen));
                    sendShapeTo(fromSocket, next);
                }
                else {
                    ShapeType next = (nextIdx2 < shapeArray.size())
                        ? shapeArray[nextIdx2++]
                        : static_cast<ShapeType>(dist(gen));
                    sendShapeTo(fromSocket, next);
                }

                return true;
            }
            else if (j["type"] == "SCORE_REQUEST") {
                std::vector<int> rows = j["rows"];
                std::vector<int> cols = j["columns"];
                int totalCleared = static_cast<int>(rows.size() + cols.size());
                double multiplier = (totalCleared > 1) ? (1.0 + 0.5 * (totalCleared - 1)) : 1.0;
                int earnedScore = static_cast<int>(totalCleared * 100 * multiplier);
                fromScore += std::min(earnedScore, 400);

                // Send response ONLY to the client who scored
                json response;
                response["type"] = "SCORE_RESPONSE";
                response["score"] = fromScore;
                sendSecure(fromSocket, response, SHARED_SECRET);

                // Send update ONLY to the opponent
                json updateMsg;
                updateMsg["type"] = "SCORE_UPDATE";
                updateMsg["opponentScore"] = fromScore;
                sendSecure(toSocket, updateMsg, SHARED_SECRET);
            }
            return true;
            };

        if (FD_ISSET(clientSocket1, &readfds)) {
            if (!processClient(clientSocket1, clientSocket2, score1, clientID1, clientID2))
                break;
        }
        if (FD_ISSET(clientSocket2, &readfds)) {
            if (!processClient(clientSocket2, clientSocket1, score2, clientID2, clientID1))
                break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(waitingMutex);
        waitingClients.erase(clientID1);
        waitingClients.erase(clientID2);
    }

    std::cout << "Session between client " << clientID1 << " and client " << clientID2 << " ended.\n";
    score1 = 0;
    score2 = 0;

    json sessionEndMsg;
    sessionEndMsg["type"] = "SESSION_OVER";
    sessionEndMsg["message"] = "Session ended. Press START_GAME to play again.";
    sendSecure(clientSocket1, sessionEndMsg, SHARED_SECRET);
    sendSecure(clientSocket2, sessionEndMsg, SHARED_SECRET);

    sessionActive->store(false);
    sessionOver.store(true);
}

//----------------------------------------------------------------------------
// Updated clientHandler now runs an outer loop so that after each session the thread resumes listening for a new START_GAME.
void clientHandler(int client_socket, int clientID) {
    {
        std::lock_guard<std::mutex> lock(clientStatesMutex);
        clientStates[clientID] = ClientInfo();
    }
    while (true) {
        {
            std::lock_guard<std::mutex> lock(waitingMutex);
            for (auto it = waitingClients.begin(); it != waitingClients.end(); ) {
                if (!isSocketAlive(it->second.clientSocket)) {
                    std::cerr << "Pruning disconnected client " << it->first << std::endl;
                    it = waitingClients.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
        // Clear any stale waiting state.
        {
            std::lock_guard<std::mutex> lock(waitingMutex);
            waitingClients.erase(clientID);
        }
        // Wait for the client to send "START_GAME".
        while (true) {
            json j;
            if (!recvSecure(client_socket, j, SHARED_SECRET, clientID)) {
                std::cerr << "Client " << clientID << " disconnected or error occurred." << std::endl;

                // Clean up client state
                {
                    std::lock_guard<std::mutex> lock(clientStatesMutex);
                    clientStates.erase(clientID);
                }

#ifdef _WIN32
                closesocket(client_socket);
#else
                close(client_socket);
#endif
                return;
            }

            if (j.contains("type") && j["type"] == "START_GAME") {
                std::cout << "Client " << clientID << " requested to start a game." << std::endl;
                break;
            }
            else {
                std::cerr << "[Server] Unknown message from client " << clientID << ": " << j.dump() << "\n";
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
                json waitMsg;
                waitMsg["type"] = "WAITING";
                waitMsg["message"] = "Waiting for another client to join...";
                sendSecure(client_socket, waitMsg, SHARED_SECRET);
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
        json startMsg;
        startMsg["type"] = "START_GAME";
        sendSecure(partnerSocket, startMsg, SHARED_SECRET);
        sendSecure(client_socket, startMsg, SHARED_SECRET);

        json tostartMsg;
        tostartMsg["type"] = "Tostart";
        tostartMsg["bool"] = true;
        sendSecure(partnerSocket, tostartMsg, SHARED_SECRET);
        sendSecure(client_socket, tostartMsg, SHARED_SECRET);

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
    ignoreSigpipe();
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