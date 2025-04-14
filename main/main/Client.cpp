#include "Client.hpp"
#include "Discovery.hpp"  // Assumes discoverServer() is declared here.
#include "Tetromino.hpp"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <nlohmann/json.hpp>  // Make sure to include this for JSON parsing
#include "Main.hpp"
#include "Menu.hpp"
#include "Audio.hpp"

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

namespace Client {
    string ScoreBuffer;
    atomic<bool> client_running(true);
    int client_socket = -1;
    string TimerBuffer = "";
    std::vector<int> shape = { 0,1,2 };
    bool startperm = false;
    std::atomic<bool> waitingForSession(false);


    // Handles receiving messages from the server continuously.
    void handle_server(int client_socket) {
        char buffer[1024];
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

                        json j = json::parse(jsonStr);
                        if (j.contains("type")) {
                            string msgType = j["type"];
                            if (msgType == "TIME_UPDATE") {
                                TimerBuffer = j["time"];
                            }
                            else if (msgType == "GAME_OVER") {
                                Audio::PlaySoundFile("Assets/Sounds/GameOver.mp3");
                                // Convert score to string (ensure proper conversion if j["score"] isn't already a string)
                                std::string finalScore = j["score"].dump();
                                std::string finalMessage = "Game Over! Your score: " + finalScore;

                                // Display the message box. 
                                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", finalMessage.c_str(), NULL);
                                client_running = false;
                                break;
                            }
                            else if (msgType == "SCORE_RESPONSE") {
                                ScoreBuffer = jsonStr;
                            }
                            else if (msgType == "SCORE_UPDATE") {
                                int oppScore = j["opponentScore"];
                                RecieveOpponentScore(oppScore);
                            }
                            else if (msgType == "SHAPE_ASSIGN" || msgType == "NEW_SHAPE") {
                                int shapeType = j["shapeType"];
                                std::cout << "Received shape type: " << shapeType << "\n";
                                shape.push_back(static_cast<int>(shapeType));
                                // Convert to your Tetromino class or store the shape
                            }
                            else if (msgType == "Tostart") {
                                bool start = j["bool"];
                                std::cout << "Received start: " << start << "\n";
                                startperm = true;
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
            j["blocks"].push_back({ {"x", block.x}, {"y", block.y}, {"block type", "d"} });
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
        std::cout << "Connecting to server..." << endl;
        if (connect(client_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            cerr << "Connection failed" << endl;
            return;
        }
        std::cout << "Connected to server" << endl;
        thread handle_thread(handle_server, client_socket);
        while (client_running) {
            this_thread::sleep_for(chrono::milliseconds(100));
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

    int Client::sendClearedLinesAndGetScore(const std::vector<int>& rows, const std::vector<int>& cols) {
        if (client_socket < 0) {
            std::cerr << "[Client] Invalid socket before send!\n";
            return 0;
        }

        json j;
        j["type"] = "SCORE_REQUEST";
        j["rows"] = rows;
        j["columns"] = cols;
        std::string message = j.dump();

        std::cout << "[Client] Sending SCORE_REQUEST...\n";
        std::cout << "[Debug] Sending on socket: " << client_socket << "\n";
        std::cout << "[Debug] Message: " << message << "\n";

        //StopResponceTaking = true; // Optional if needed elsewhere
        send(client_socket, message.c_str(), message.size(), 0);

        // Wait briefly for ScoreBuffer to be updated
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        try {
            std::string msg = ScoreBuffer;

            // Optional: Trim to JSON
            size_t end = msg.find("}") + 1;
            if (end != std::string::npos && end < msg.size()) {
                msg = msg.substr(0, end);
            }

            json response = json::parse(msg);
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

    // New: runClient() simply checks if the server IP was discovered and calls start_client().
    void runClient() {
        if (!server_ip.empty()) {
            start_client(server_ip);
        }
        else {
            cerr << "Could not find server automatically!" << endl;
			exit(0);
        }
    }

    void resetClientState() {
        client_running = true;
        TimerBuffer = "";
        ScoreBuffer = "";
        startperm = false;
        waitingForSession = false;
        StopResponceTaking = false;
        shape.clear();
        client_socket = -1;
    }
}
