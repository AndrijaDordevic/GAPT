#include "Window.hpp"
#include "Menu.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <SDL3_ttf/SDL_ttf.h>
#include <thread>  // For running client in a separate thread
#include "Client.hpp"
#include <string>
#include <fstream>
#include "Texture.hpp"

#define OUTPUT_FILE "server_ip.txt"  // File to store server IP

using namespace std;

TTF_Font* font = nullptr;

SDL_Color textColor = { 0, 0, 0, 255 };

bool closed = false;

// Menu item structure
struct MenuItem {
	SDL_FRect rect;
	string label;
	bool isHovered;
	SDL_Texture* textTexture;
	int textWidth;
	int textHeight;
};
bool running = true;

// Create window 
SDL_Window* windowm = SDL_CreateWindow("Main Menu", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY);

// Create renderer for window
SDL_Renderer* rendererm = SDL_CreateRenderer(windowm, NULL);

// Initialize menu items: Rect pos and size, text and initial isHovered value.
vector<MenuItem> menuItems = {
	{{(WINDOW_WIDTH - 400) / 2, 500, 400, 100}, "Start Game", false, nullptr, 0, 0}, // Start Game
	{{(WINDOW_WIDTH - 400) / 2, 650, 400, 100}, "Options", false, nullptr, 0, 0}, // Options
	{{(WINDOW_WIDTH - 400) / 2, 800, 400, 100}, "Exit", false, nullptr, 0, 0}  // Exit
};

// Function to load font texture
bool loadFontTexture() {
	TTF_Font* font = TTF_OpenFont("Assets\\Sterion-BLLld.ttf", 24);
	if (!font) {
		cerr << "Failed to load font." << SDL_GetError << endl;
		return false;
	}

	for (auto& item : menuItems) {
		SDL_Surface* textSurface = TTF_RenderText_Blended(font, item.label.c_str(), item.label.length(), textColor);
		if (!textSurface) {
			cerr << "Failed to create text surface." << SDL_GetError() << endl;
			return false;
		}

		item.textTexture = SDL_CreateTextureFromSurface(rendererm, textSurface);
		item.textWidth = textSurface->w;
		item.textHeight = textSurface->h;
		SDL_DestroySurface(textSurface);

		if (!item.textTexture) {
			cerr << "Failed to create text texture." << SDL_GetError() << endl;
			return false;
		}
	}
	return true;
}

// Cleanup textures
void cleanupTextures() {
	for (auto& item : menuItems) {
		if (item.textTexture) {
			SDL_DestroyTexture(item.textTexture);
			item.textTexture = nullptr;
		}
	}
}

// Render menu
void renderMenu(SDL_Renderer* renderer) {
	SDL_Texture* menuTexture = LoadMenuTexture(renderer);
	SDL_Texture* menuOptionTexture = LoadMenuOptionTexture(renderer);
	SDL_Texture* menuOptionTextureS = LoadMenuOptionTextureSelected(renderer);
	SDL_RenderTexture(renderer, menuTexture, nullptr, nullptr);
	for (const auto& item : menuItems) {
		item.isHovered ? SDL_RenderTexture(renderer, menuOptionTextureS, nullptr, &item.rect) : SDL_RenderTexture(renderer, menuOptionTexture, nullptr, &item.rect);


		if (item.textTexture) {
			SDL_FRect textRect = {
				item.rect.x + (item.rect.w - item.textWidth) / 2,
				item.rect.y + (item.rect.h - item.textHeight) / 2,
				static_cast<float>(item.textWidth),
				static_cast<float>(item.textHeight)
			};
			SDL_RenderTexture(renderer, item.textTexture, nullptr, &textRect);

		}
	}
}

// Check if mouse is over a rectangle
bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect) {
	return (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
		mouseY >= rect.y && mouseY <= rect.y + rect.h);
}


// Function to read the server IP from the saved file
string readServerIP() {
	ifstream infile(OUTPUT_FILE);  // Open the file in input mode
	if (!infile) {
		cerr << "Client: Failed to open " << OUTPUT_FILE << " for reading." << endl;
		return "";  // Return empty string if file can't be read
	}

	string serverIP;
	getline(infile, serverIP);  // Read the IP address as a string

	infile.close();  // Close the file after reading
	return serverIP;
}

// Function to run the client code (in a separate thread)
void runClient() {
	// Read the server IP from the file
	string server_ip = discoverServer();
	if (!server_ip.empty()) {
		start_client(server_ip);
	}
	else {
		cerr << "Could not find server automatically!\n";
	}


	// If the IP is empty, print an error and stop the client
	if (server_ip.empty()) {
		cerr << "Client: No server IP found. Please ensure the listener has saved it." << endl;
		return;
	}

	// At this point, we have a valid server IP, so we can call start_client() with the IP
	cout << "Client: Using server IP: " << server_ip << "\n";

	// Start the client with the IP address
	start_client(server_ip);  // Call your client start function here
}

// Main menu loop
int runMenu(SDL_Window* window, SDL_Renderer* renderer) {
	SDL_Event event;

	// Initialize TTF
	if (TTF_Init() == false) {
		cerr << "Failed to initialize TTF." << SDL_GetError << endl;
		return 1;
	}

	if (!loadFontTexture()) {
		cerr << "Failed to load font textures." << endl;
		TTF_Quit();
		return 1;
	}
	// Main loop
	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
				// Get mouse coordinates
				float mouseX = static_cast<float>(event.motion.x);
				float mouseY = static_cast<float>(event.motion.y);

				// Check menu items
				for (size_t i = 0; i < menuItems.size(); i++) {
					menuItems[i].isHovered = isMouseOver(mouseX, mouseY, menuItems[i].rect);

					if (menuItems[i].isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
						if (i == 2) {
							exit(EXIT_SUCCESS); // Exit
						}
						else if (i == 0) {
							// Start game
							cout << "Starting game..." << endl;
							// Start the client in a separate thread
							thread clientThread(runClient);
							clientThread.detach(); // Detach the thread to keep running
							running = false; // Close the menu after starting the game
						}
						else if (i == 1) {
							// Options
							cout << "Options..." << endl;
						}
					}
				}
			}
		}

		// Clear screen
		SDL_SetRenderDrawColor(rendererm, 255, 255, 255, 255);
		SDL_RenderClear(rendererm);

		// Render menu
		renderMenu(rendererm);
		SDL_RenderPresent(rendererm);
		SDL_Delay(16);
	}

	// Cleanup
	cleanupTextures();
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_DestroyRenderer(rendererm);
	SDL_DestroyWindow(windowm);
	SDL_Quit();
	return 0;
}
