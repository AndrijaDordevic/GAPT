// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <atomic>
#include <string>
#include <nlohmann/json.hpp>
#include "Tetromino.hpp"


namespace Client {
	// Global variables for client state and configuration.
    extern std::vector<int> shape;
    extern std::atomic<bool> client_running;
    extern std::string TimerBuffer;
    extern bool startperm;
    extern bool gameOver;
    extern bool initialized;
    extern int spawnedCount;


    // Function to start the client.
    void start_client(const std::string& server_ip);

    // Function to handle communication with the server.
    void handle_server(int client_socket);

    // Function to run the client.
    void runClient();

    // New function to notify the server to start a game session.
    bool notifyStartGame();

    bool sendDragCoordinates(const Tetromino& tetromino);
    int sendClearedLinesAndGetScore(const std::vector<int>& rows, const std::vector<int>& cols);

    void resetClientState();

}

#endif // CLIENT_HPP
