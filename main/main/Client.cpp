#include "Client.hpp"
#include "Discovery.hpp"  
#include "Tetromino.hpp"
#include "Window.hpp"
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
#include <cstdio>
#include <mutex>


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

int id = 1;

// Get the server IP using your discovery function.
string server_ip = discoverServer();
bool StopResponceTaking;


static const std::string SHARED_SECRET = "9fbd2c3e8b4f4ea6e6321ad4c68a1fb2e0d72b89d0a4c715ffbd9b184207c17e";

static constexpr const char* SESSION_FILE = "reconnect.dat";

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
    std::atomic<bool> resumeNow(false);       // flag set by STATE_SNAPSHOT
    std::mutex        snapMtx;               // protects lastSnapshot
    nlohmann::json    lastSnapshot;          // most-recent snapshot
    bool Client::startSent = false;

    int myScore = 0;     
    size_t nextShapeIdx = 0;
    std::vector<nlohmann::json> lockedBlocks;   // grid cells from snapshot
    atomic<bool> inSession(false);
    std::atomic<bool> StopResponceTaking{ false };
    bool attemptReconnect(uint64_t token, int cid);

    uint64_t sessionToken = 0;   // the token from SESSION_START
    int      myClientID = -1;  // the ID the server assigned us


    bool displayWaitingMessage = false;

    bool saveSessionInfo(uint64_t token, int clientID) {
        std::ofstream ofs(SESSION_FILE, std::ios::binary);
        if (!ofs) return false;
        ofs.write(reinterpret_cast<const char*>(&token), sizeof(token));
        ofs.write(reinterpret_cast<const char*>(&clientID), sizeof(clientID));
        return bool(ofs);
    }
    bool loadSessionInfo(uint64_t& tok, int& cid)
    {
        std::ifstream f(SESSION_FILE, std::ios::binary);
        if (!f) return false;

        f.read(reinterpret_cast<char*>(&tok), sizeof(tok));
        f.read(reinterpret_cast<char*>(&cid), sizeof(cid));

        /* reject obviously bad data */
        if (tok == 0 || cid <= 0 || cid > 10'000)   
        {
            f.close();
            std::remove(SESSION_FILE);           
            return false;
        }
        return bool(f);
    }
    void clearSessionInfo() {
        std::remove(SESSION_FILE);
    }


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
        RecieveOpponentScore(0);
		startSent = false;
    }

    static bool validateHMAC(json& j, const std::string& secret) {
        if (!j.contains("hmac")) {
            std::cerr << "No HMAC in message – rejecting\n";
            return false;
        }
        std::string receivedTag = j["hmac"];
        // Remove it for re-computing
        j.erase("hmac");
        std::string bodyStr = j.dump();
        // Compute expected
        std::string expectedTag = computeHMAC(bodyStr, secret);
        if (!hmacEquals(receivedTag, expectedTag)) {
            std::cerr << "HMAC verification failed – possible tampering\n";
            return false;
        }
        return true;
    }

    void handle_server(int client_socket) {
        std::string accumulatedMessage;
        char buffer[4096];

        while (client_running && !StopResponceTaking) {
            //read once per loop
            int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

            if (bytes_received <= 0) {
                // real disconnect?
#ifdef _WIN32
                int err = WSAGetLastError();
                bool wouldBlock = (err == WSAEWOULDBLOCK);
#else
                int err = errno;
                bool wouldBlock = (err == EAGAIN || err == EWOULDBLOCK);
#endif

                if (!wouldBlock) {
                    // If we're in a session, try to reconnect
                    if (inSession.load()) {
                        std::cerr << "Lost connection – attempting reconnect...\n";
                        if (!attemptReconnect(sessionToken, myClientID)) {
                            std::cerr << "Reconnect failed – exiting\n";
                            client_running = false;
                            break;
                        }
                        // on success, loop back and keep reading
                        continue;
                    }
                    // if we're _not_ in a session, just pause and keep the socket open
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }
            else {
                // accumulate raw bytes
                accumulatedMessage.append(buffer, bytes_received);

                // pull out each '\n'-terminated JSON blob
                size_t nl;
                while ((nl = accumulatedMessage.find('\n')) != std::string::npos) {
                    std::string raw = accumulatedMessage.substr(0, nl);
                    accumulatedMessage.erase(0, nl + 1);

                    std::cout << "[Debug] JSON String: " << raw << "\n"; 

                    try {
                        json j = json::parse(raw);

                        if (!validateHMAC(j, SHARED_SECRET)) {
                            continue;
                        }
                        if (j.contains("type")) {
                            std::string msgType = j["type"];
                            if (msgType == "TIME_UPDATE") {
                                TimerBuffer = j["time"];
                            }
                            else if (msgType == "STATE_SNAPSHOT") {
                                // 1) Always stash the new snapshot under lock
                                {
                                    std::lock_guard<std::mutex> lk(snapMtx);
                                    lastSnapshot = j;
                                }

                                // 2) Only trigger the main thread to re-enter the game loop once
                                if (!inSession.load()) {
                                    resumeNow.store(true);
                                    inSession.store(true);
                                }

                                // 3) Refresh score & grid state
                                ScoreBuffer.clear();

                                lockedBlocks.clear();
                                if (j.contains("grid") && j["grid"].is_array()) {
                                    std::cout << "Grid data:" << j["grid"].dump(4) << std::endl;
                                    for (auto& cell : j["grid"])
                                        lockedBlocks.push_back(cell);
                                    std::cout << "Locked Blocks:" << lockedBlocks << std::endl;
                                }

                                shape.clear();
                                if (j.contains("inHand") && j["inHand"].is_array()) {
                                    for (auto& sh : j["inHand"])
                                        shape.push_back(sh.get<int>());
                                }

                                if (j.contains("yourScore")) {
                                    myScore = j["yourScore"].get<int>();
                                    ScoreBuffer = json{
                                      {"type", "SCORE_RESPONSE"},
                                      {"score", myScore}
                                    }.dump();
                                }

                                if (j.contains("opponentScore")) {
                                    RecieveOpponentScore(j["opponentScore"].get<int>());
                                }

                                // 4) Signal the menu thread that the menu window should close
                                state::running = false;
                                state::closed = false;
                            }


                            else if (msgType == "SCORE_RESPONSE") {
                                ScoreBuffer = j.dump();
                            }
                            else if (msgType == "SCORE_UPDATE") {
                                RecieveOpponentScore(j["opponentScore"]);
                            }
                            else if (msgType == "SHAPE_ASSIGN"
                                || msgType == "NEW_SHAPE") {
                                int shapeType = j["shapeType"];
                                std::cout << "Received shape type: "
                                    << shapeType << "\n";
                                shape.push_back(shapeType);
                            }
                            else if (msgType == "Tostart") {
                                bool start = j["bool"];
                                std::cout << "Received start: " << start << "\n";
                                startperm = true;
                                tetrominos.clear();
                                placedTetrominos.clear();
                                inSession.store(true);
                            }
                            else if (msgType == "SESSION_START") {
                                sessionToken = j["sessionToken"].get<uint64_t>();
                                myClientID = j["clientID"].get<int>();        
                                saveSessionInfo(sessionToken, myClientID);     
                                std::cout << "Session started: token=" << sessionToken
                                    << "  yourID=" << myClientID << "\n";
                                inSession.store(true);
                            }
                            else if (msgType == "SESSION_ABORT") {
                                inSession.store(false);
                                Audio::PlaySoundFile(
                                    "Assets/Sounds/GameOver.mp3");
                                SDL_ShowSimpleMessageBox(
                                    SDL_MESSAGEBOX_INFORMATION,
                                    "Game Over",
                                    "Game over! You Win! Your opponent disconnected.",
                                    NULL
                                );
                                gameOver = true;
                                resetClientState();
                                accumulatedMessage.clear();
                                clearSessionInfo();
                            }
                            else if (msgType == "GAME_OVER") {
                                inSession.store(false);
                                Audio::PlaySoundFile(
                                    "Assets/Sounds/GameOver.mp3");
                                std::string finalScore = j["score"].dump();
                                std::string finalOutcome =
                                    j["outcome"].get<std::string>();
                                std::string finalMessage;
                                if (finalOutcome == "draw!")
                                    finalMessage =
                                    "Game Over! It's a draw! Your score: "
                                    + finalScore;
                                else
                                    finalMessage =
                                    "Game Over! You " + finalOutcome
                                    + " Your score: " + finalScore;
                                SDL_ShowSimpleMessageBox(
                                    SDL_MESSAGEBOX_INFORMATION,
                                    "Game Over",
                                    finalMessage.c_str(),
                                    NULL
                                );
                                gameOver = true;
                                resetClientState();
                                accumulatedMessage.clear();
                                clearSessionInfo(); 
                            }
                            else if (msgType == "SESSION_OVER") {
                                inSession.store(false);
                                std::cout << "Session Ended!\n";
                                resetClientState();
                                accumulatedMessage.clear();
                                clearSessionInfo();
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[Client] JSON parse error: "
                            << e.what() << "\n";
                    }
                } 
            } 

            std::this_thread::sleep_for(
                std::chrono::milliseconds(50));
        } 
    }

    bool tryReconnect(int sock)
    {
        std::ifstream f(SESSION_FILE);
        uint64_t token; int id;
        if (!(f >> token >> id)) return false;

        nlohmann::json r = {
            {"type","RECONNECT"},
            {"token",token},
            {"id",id}
        };
        std::string body = r.dump();
        std::string tag = computeHMAC(body, SHARED_SECRET);
        r["hmac"] = tag;
        std::string out = r.dump() + "\n";
        return send(sock, out.c_str(), out.size(), 0) == (int)out.size();
    }

    void shutdownConnection()
    {     

        client_running = false;       
        StopResponceTaking = true;

        if (client_socket >= 0) {
#ifdef _WIN32
            shutdown(client_socket, SD_BOTH);
            closesocket(client_socket);
#else
            shutdown(client_socket, SHUT_RDWR);
            close(client_socket);
#endif
            client_socket = -1;
        }
    }

    bool attemptReconnect(uint64_t token, int cid) {
        // create a fresh socket
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;

        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(PORT);
        inet_pton(AF_INET, server_ip.c_str(), &sa.sin_addr);

        if (::connect(sock, (sockaddr*)&sa, sizeof sa) < 0) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            return false;
        }

        // send the RECONNECT handshake
        json j;
        j["type"] = "RECONNECT";
        j["token"] = token;
        j["id"] = cid;
        client_socket = sock;
        if (!sendSecure(j)) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            return false;
        }
        return true;
    }

    bool notifyStartGame() {
        if (startSent) return true;

        std::cout << "Attempting to notify server to start game on IP "
            << server_ip << " and port " << PORT << endl;
        if (client_socket < 0) {
            cerr << "Client socket is not connected!" << endl;
            return false;
        }

        json j;
        j["type"] = to_string(id) + "START_GAME";
        id++;
        if (!sendSecure(j)) {
            return false;
        }

        // mark it so future calls do nothing
        startSent = true;
        return true;
    }

    bool sendDragCoordinates(const Tetromino& tetromino) {
        json j;
        j["type"] = to_string(id) + "DRAG_UPDATE";
        id++;
        j["blocks"] = json::array();
        for (const auto& block : tetromino.blocks) {
            // convert pixel → grid
            int col = (block.x - GRID_START_X) / BLOCK_SIZE;
            int row = (block.y - GRID_START_Y) / BLOCK_SIZE;
            j["blocks"].push_back({
                {"x", col},              // 0..numCols-1
                {"y", row},              // 0..numRows-1
                {"r", block.color.r},
                {"g", block.color.g},
                {"b", block.color.b}
                });
        }
        return sendSecure(j);
    }


    void start_client(const std::string& server_ip) {
        std::cout << "*Server IP: " << server_ip << "\n";

#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "[Client] WSAStartup failed!\n";
            return;
        }
#endif

        // 1) Try to resume
        {
            uint64_t tok;
            int cid;
            if (loadSessionInfo(tok, cid)) {
                std::cout << "[Client] Found saved session, attempting reconnect...\n";
                if (attemptReconnect(tok, cid)) {
                    std::cout << "[Client] Reconnected to session token=" << tok
                        << " as clientID=" << cid << "\n";
                }
                else {
                    std::cout << "[Client] Reconnect failed, starting new session.\n";
                    clearSessionInfo();
                }
            }
        }

        // 2) If we still have no socket, do a fresh connect
        if (client_socket < 0) {
            client_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (client_socket < 0) {
                std::cerr << "[Client] Socket creation failed\n";
#ifdef _WIN32
                WSACleanup();
#endif
                return;
            }

            sockaddr_in sa{};
            sa.sin_family = AF_INET;
            sa.sin_port = htons(PORT);
            if (inet_pton(AF_INET, server_ip.c_str(), &sa.sin_addr) <= 0) {
                std::cerr << "[Client] Invalid address\n";
#ifdef _WIN32
                closesocket(client_socket);
                WSACleanup();
#else
                close(client_socket);
#endif
                client_socket = -1;
                return;
            }

            std::cout << "[Client] Connecting to server...\n";
            if (connect(client_socket, (sockaddr*)&sa, sizeof sa) < 0) {
                std::cerr << "[Client] Connection failed\n";
#ifdef _WIN32
                closesocket(client_socket);
                WSACleanup();
#else
                close(client_socket);
#endif
                client_socket = -1;
                return;
            }
            std::cout << "[Client] Connected to server\n";
        }

        // 3) Launch the receive thread
        std::thread reader(handle_server, client_socket);

        // 4) Wait here until the reader sets client_running = false
        while (client_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 5) Clean up
        std::cout << "[Client] Disconnecting...\n";
#ifdef _WIN32
        closesocket(client_socket);
        WSACleanup();
#else
        close(client_socket);
#endif

        reader.join();
    }

    int Client::UpdateScore() {
        if (ScoreBuffer.empty()) {
            // No score update has arrived yet
            return 0;
        }

        try {
            json response = json::parse(ScoreBuffer);
            if (response.contains("type") && response["type"] == "SCORE_RESPONSE") {
                int score = response["score"].get<int>();
                 cout << "[Client] Score response received: " << score << "\n";
                // clear it so you don’t parse it again on the next tick

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
        j["type"] = to_string(id) + "SCORE_REQUEST";
        j["rows"] = rows;
        j["columns"] = cols;
        id++;
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