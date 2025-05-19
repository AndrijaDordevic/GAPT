#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <atomic>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Tetromino.hpp"
#include <mutex>


namespace Client {
    extern std::vector<int> shape;
    extern std::atomic<bool> client_running;
    extern std::string TimerBuffer;
    extern bool startperm;
    extern bool gameOver;
    extern bool initialized;
    extern int spawnedCount;
    extern std::atomic<bool> resumeNow;
    extern std::mutex        snapMtx;
    extern nlohmann::json    lastSnapshot;
    extern int               myScore;
    extern size_t            nextShapeIdx;
    extern std::vector<nlohmann::json> lockedBlocks;
    extern std::atomic<bool> inSession;
    extern std::atomic<bool> StopResponceTaking;
    extern std::atomic<bool> waitingForSession;
    extern bool displayWaitingMessage;   

    // score/tetromino APIs
    int UpdateScore();
    bool notifyStartGame();
    bool sendDragCoordinates(const Tetromino&);
    int sendClearedLinesAndGetScore(const std::vector<int>&, const std::vector<int>&);

    // session persistence
    bool loadSessionInfo(uint64_t& token, int& clientID);
    bool saveSessionInfo(uint64_t token, int clientID);
    void clearSessionInfo();

    // the reconnect API
    bool attemptReconnect(uint64_t token, int cid);

    void resetClientState();
    void handle_server(int sock);
    void start_client(const std::string& server_ip);
    void runClient();
	void shutdownConnection();

}

#endif
