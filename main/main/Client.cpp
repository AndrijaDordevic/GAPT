#include "Client.hpp"
#include "Discovery.hpp"  // Assumes discoverServer() is declared here.
#include "Tetromino.hpp"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Main.hpp"
#include "Menu.hpp"
#include "Audio.hpp"
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>

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
bool StopResponceTaking;

static const std::string SHARED_SECRET = "my?very?strong?key";

namespace Client {
    string ScoreBuffer;
    atomic<bool> client_running(true);
    int client_socket = -1;
    string TimerBuffer = "";
    std::vector<int> shape = { };
    bool startperm = false;
    std::atomic<bool> waitingForSession(false);
    bool gameOver = false;
    bool initialized = false;
    int spawnedCount = 0;
    atomic<bool> inSession(false);

    static std::string computeHMAC(const std::string& data, const std::string& secret) {
        unsigned char* result = HMAC(
            EVP_sha256(),
            reinterpret_cast<const unsigned char*>(secret.data()), secret.size(),
            reinterpret_cast<const unsigned char*>(data.data()), data.size(),
            nullptr, nullptr
        );

        std::ostringstream hex;
        hex << std::hex << std::setfill('0');
        for (int i = 0; i < 32; ++i) {
            hex << std::setw(2) << static_cast<int>(result[i]);
        }
        return hex.str();
    }

    static bool hmacEquals(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        unsigned char diff = 0;
        for (size_t i = 0; i < a.size(); ++i)
            diff |= a[i] ^ b[i];
        return diff == 0;
    }

    bool sendSecure(const json& j) {
        if (client_socket < 0) {
            cerr << "Client socket is not connected!" << endl;
            return false;
        }

        std::string body = j.dump();
        std::string tag = computeHMAC(body, SHARED_SECRET);
        json withTag = j;
        withTag["hmac"] = tag;
        std::string out = withTag.dump() + "\n";

        int sent = send(client_socket, out.c_str(), out.size(), 0);
        if (sent <= 0) {
#ifdef _WIN32
            int error = WSAGetLastError();
#else
            int error = errno;
#endif
            cerr << "Failed to send secure message! Error code: " << error << endl;
            return false;
        }
        return true;
    }

    bool recvSecure(json& j) {
        char buffer[4096];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            return false; // Connection closed or error
        }
        buffer[bytes_received] = '\0';

        try {
            j = json::parse(buffer);

            // Verify HMAC if present
            if (j.contains("hmac")) {
                std::string receivedTag = j["hmac"];
                json body = j; // Make a copy
                body.erase("hmac"); // Remove the tag for verification
                std::string bodyStr = body.dump();

                // Compute expected HMAC
                std::string expectedTag = computeHMAC(bodyStr, SHARED_SECRET);

                // Constant-time comparison
                if (!hmacEquals(receivedTag, expectedTag)) {
                    std::cerr << "HMAC verification failed - possible tampering\n";
                    return false;
                }

                // Remove HMAC from the parsed JSON since it's verified
                j.erase("hmac");
                return true;
            }
            else {
                std::cerr << "No HMAC in message - rejecting\n";
                return false;
            }
        }
        catch (...) {
            std::cerr << "Failed to parse JSON message\n";
            return false;
        }
    }

    void resetClientState() {
        client_running = true;
        TimerBuffer = "";
        ScoreBuffer = "";
        startperm = false;
        waitingForSession = false;
        StopResponceTaking = false;
        displayWaitingMessage = false;
        shape.clear();
        state::running = true;
        state::closed = false;
        gameOver = false;
        initialized = false;
        spawnedCount = 0;
        inSession = false;
    }

    void handle_server(int client_socket) {
        char buffer[800];
        string accumulatedMessage = "";  // Accumulate partial messages here

        while (client_running && !StopResponceTaking) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                string msg(buffer);
                accumulatedMessage += msg;  // Accumulate the message

                try {
                    size_t start = 0;
                    while ((start = accumulatedMessage.find('{')) != std::string::npos) {
                        size_t end = accumulatedMessage.find('}', start);
                        if (end == std::string::npos) break;

                        std::string jsonStr = accumulatedMessage.substr(start, end - start + 1);
                        accumulatedMessage = accumulatedMessage.substr(end + 1);  // Move to the next JSON

                        std::cout << "[Debug] JSON String: " << jsonStr << std::endl;  // Print the JSON string

                        json j;
                        if (!recvSecure(j)) {
                            std::cerr << "Invalid or tampered message received\n";
                            continue;
                        }

                        if (j.contains("type")) {
                            string msgType = j["type"];
                            if (msgType == "TIME_UPDATE") {
                                TimerBuffer = j["time"];
                            }
                            else if (msgType == "SCORE_RESPONSE") {
                                ScoreBuffer = j.dump();
                            }
                            else if (msgType == "SCORE_UPDATE") {
                                int oppScore = j["opponentScore"];
                                RecieveOpponentScore(oppScore);
                            }
                            else if (msgType == "SHAPE_ASSIGN" || msgType == "NEW_SHAPE") {
                                int shapeType = j["shapeType"];
                                std::cout << "Received shape type: " << shapeType << "\n";
                                shape.push_back(static_cast<int>(shapeType));
                            }
                            else if (msgType == "Tostart") {
                                bool start = j["bool"];
                                std::cout << "Received start: " << start << "\n";
                                startperm = true;
                                tetrominos.clear();
                                placedTetrominos.clear();
                                inSession.store(true);
                            }
                            else if (msgType == "SESSION_ABORT") {
                                inSession.store(false);
                                Audio::PlaySoundFile("Assets/Sounds/GameOver.mp3");
                                std::string finalMessage = "Game over! You Win! Your opponent disconnected.";
                                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", finalMessage.c_str(), NULL);
                                gameOver = true;
                                resetClientState();
                                accumulatedMessage.clear();
                            }
                            else if (msgType == "GAME_OVER") {
                                inSession.store(false);
                                Audio::PlaySoundFile("Assets/Sounds/GameOver.mp3");
                                std::string finalScore = j["score"].dump();
                                std::string finalOutcome = j["outcome"].get<std::string>();
                                std::string finalMessage;

                                if (finalOutcome == "draw!") {
                                    finalMessage = "Game Over! Its a " + finalOutcome + " Your score: " + finalScore;
                                }
                                else {
                                    finalMessage = "Game Over! You " + finalOutcome + " Your score: " + finalScore;
                                }

                                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", finalMessage.c_str(), NULL);
                                gameOver = true;
                                resetClientState();
                                accumulatedMessage.clear();
                                break;
                            }
                            else if (msgType == "SESSION_OVER") {
                                inSession.store(false);
                                std::cout << "Session Ended!" << "\n";
                                resetClientState();
                                accumulatedMessage.clear();
                                continue;
                            }
                        }
                    }
                }
                catch (const std::exception& e) {
                    cerr << "[Client] JSON parse error: " << e.what() << "\n";
                }
            }
            else if (bytes_received == 0) {
                std::cout << "Server disconnected.\n";
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
        std::cout << "Attempting to notify server to start game on IP "
            << server_ip << " and port " << PORT << endl;
        if (client_socket < 0) {
            cerr << "Client socket is not connected!" << endl;
            return false;
        }

        json j;
        j["type"] = "START_GAME";
        return sendSecure(j);
    }

    bool sendDragCoordinates(const Tetromino& tetromino) {
        json j;
        j["type"] = "DRAG_UPDATE";
        j["blocks"] = json::array();
        for (const auto& block : tetromino.blocks) {
            j["blocks"].push_back({ {"x", block.x}, {"y", block.y}, {"block type", "d"} });
        }
        return sendSecure(j);
    }

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
        std::cout << "Connecting to server..." << endl;
        if (connect(client_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            cerr << "Connection failed" << endl;
            return;
        }
        std::cout << "Connected to server" << endl;
        thread handle_thread(handle_server, client_socket);
        while (client_running) {
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
        std::cout << "Disconnecting..." << endl;
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

    int Client::UpdateScore() {
        try {
            json response = json::parse(ScoreBuffer);
            std::cout << "[Client] Parsed ScoreBuffer JSON: " << response.dump() << "\n";

            if (response.contains("type") && response["type"] == "SCORE_RESPONSE") {
                int score = response["score"].get<int>();
                std::cout << "[Client] Extracted score: " << score << "\n";
                return score;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[Client] JSON parse failed from ScoreBuffer: " << e.what() << "\n";
        }
        return 0;
    }

    int Client::sendClearedLinesAndGetScore(const std::vector<int>& rows, const std::vector<int>& cols) {
        if (client_socket < 0) {
            std::cerr << "[Client] Invalid socket before send!\n";
            return 0;
        }

        json j;
        j["type"] = "SCORE_REQUEST";
        j["rows"] = rows;
        j["columns"] = cols;

        if (!sendSecure(j)) {
            std::cerr << "[Client] Failed to send SCORE_REQUEST\n";
            return 0;
        }

        return 0; // Actual score will be received asynchronously
    }

    void runClient() {
        if (!server_ip.empty()) {
            start_client(server_ip);
        }
        else {
            cerr << "Could not find server automatically!" << endl;
            exit(0);
        }
    }
}