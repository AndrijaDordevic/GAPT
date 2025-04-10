// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <atomic>
#include <string>
#include <nlohmann/json.hpp>
#include "Tetromino.hpp"


namespace Client {
    extern std::atomic<bool> client_running;

    // Function to start the client.
    void start_client(const std::string& server_ip);

    // Function to handle communication with the server.
    void handle_server(int client_socket);

    // Function to run the client.
    void runClient();

    // New function to notify the server to start a game session.
    bool notifyStartGame();

    bool sendDragCoordinates(const Tetromino& tetromino);
}

#endif // CLIENT_HPP
