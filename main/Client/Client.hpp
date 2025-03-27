#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <atomic>
#include <string>

// Declare the client_running variable as extern.
extern std::atomic<bool> client_running;

// Function to start the client.
void start_client(const std::string& server_ip);

std::string discoverServer();

// Function to handle communication with the server.
void handle_server(int client_socket);

#endif // CLIENT_HPP
