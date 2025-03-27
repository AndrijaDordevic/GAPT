#include <iostream>
#include <cstring>  // For memset
#include <cstdlib>  // For exit()
#include <thread>   // For multi-threading
#include <chrono>   // For sleep_for
#include <string>
#include "Client.hpp"
#include <atomic>

#ifdef _WIN32  // Windows-specific headers
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // Link with Windows sockets library
#else  // Linux/macOS headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>  // For close()
#endif

#define PORT 1235 // Port to listen for broadcasts
#define TCP_PORT 12346        // Port to connect via TCP  // Port to connect to

// Global atomic flag to manage client shutdown
std::atomic<bool> client_running(true);

// Function to handle receiving messages from the server
void handle_server(int client_socket) {
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));  // Clear buffer

	// Receive a message from the server
	int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
	if (bytes_received > 0) {
		buffer[bytes_received] = '\0';  // Ensure null termination
		std::cout << "Server says: " << buffer;  // Print server message
	}
	else if (bytes_received == 0) {
		std::cout << "Server disconnected.\n";
	}
	else {
		std::cerr << "Failed to receive data from server\n";
	}
}

std::string discoverServer() {
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed\n";
		return "";
	}
#endif

	// Create UDP socket
	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_socket < 0) {
		std::cerr << "Failed to create UDP socket: ";
#ifdef _WIN32
		std::cerr << WSAGetLastError();
#else
		std::cerr << strerror(errno);
#endif
		std::cerr << std::endl;
#ifdef _WIN32
		WSACleanup();
#endif
		return "";
	}

	// Enable address reuse - CRITICAL FIX
	int reuse = 1;
	if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&reuse, sizeof(reuse)) < 0) {
		std::cerr << "Failed to set SO_REUSEADDR: ";
#ifdef _WIN32
		std::cerr << WSAGetLastError();
#else
		std::cerr << strerror(errno);
#endif
		std::cerr << std::endl;
#ifdef _WIN32
		closesocket(udp_socket);
		WSACleanup();
#else
		close(udp_socket);
#endif
		return "";
	}

	// Configure listening address
	sockaddr_in listen_addr{};
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(PORT);  // UDP broadcast port
	listen_addr.sin_addr.s_addr = INADDR_ANY;

	// Bind the socket
	if (bind(udp_socket, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
		std::cerr << "Binding UDP socket failed: ";
#ifdef _WIN32
		std::cerr << WSAGetLastError();
#else
		std::cerr << strerror(errno);
#endif
		std::cerr << std::endl;
#ifdef _WIN32
		closesocket(udp_socket);
		WSACleanup();
#else
		close(udp_socket);
#endif
		return "";
	}

	char buffer[1024];
	sockaddr_in sender_addr{};
	socklen_t sender_len = sizeof(sender_addr);

	std::cout << "Waiting for server broadcast...\n";

	// Set a timeout (5 seconds)
#ifdef _WIN32
	DWORD timeout = 5000;
	setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#else
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

	int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
		(struct sockaddr*)&sender_addr, &sender_len);

	std::string result;
	if (bytes_received > 0) {
		buffer[bytes_received] = '\0';
		std::cout << "Received: " << buffer << std::endl;

		std::string received_data(buffer);
		if (received_data.find("SERVER_IP:") == 0) {
			size_t first_colon = received_data.find(":");
			size_t second_colon = received_data.find(":", first_colon + 1);
			if (first_colon != std::string::npos && second_colon != std::string::npos) {
				result = received_data.substr(first_colon + 1, second_colon - first_colon - 1);
			}
		}
	}
	else {
		std::cerr << "No server broadcast received within timeout period\n";
	}

#ifdef _WIN32
	closesocket(udp_socket);
	WSACleanup();
#else
	close(udp_socket);
#endif

	return result;
}

// Function to initiate a connection to the server
void start_client(const std::string& server_ip) {

	std::cout << "*Server IP:  " << server_ip << std::endl;
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	int client_fd;
	struct sockaddr_in server_addr;

	// Create socket
	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd == -1) {
		std::cerr << "Socket creation failed\n";
		return;
	}

	// Configure server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	// Convert server IP address to binary
	if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
		std::cerr << "Invalid address\n";
		return;
	}

	// Connect to the server
	std::cout << "Connecting to server...\n";
	if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		std::cerr << "Connection failed\n";
		return;
	}

	std::cout << "Connected to server\n";

	// Start a thread to handle communication
	std::thread handle_thread(handle_server, client_fd);

	// Keep the client running
	while (client_running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::cout << "Disconnecting...\n";
	closesocket(client_fd);
	client_running = false;
	handle_thread.join();

#ifdef _WIN32
	WSACleanup();
#endif
}