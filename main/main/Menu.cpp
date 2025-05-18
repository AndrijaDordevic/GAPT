#include "Window.hpp"
#include "Menu.hpp"
#include "Audio.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <SDL3_ttf/SDL_ttf.h>
#include <thread>   
#include "Client.hpp"
#include "Texture.hpp"
#include "Discovery.hpp"
#include <string>
#include <windows.h>
#include "TextRender.hpp"


#define OUTPUT_FILE "server_ip.txt"  // File to store server IP

using namespace std;

TTF_Font* font = nullptr;

SDL_Color textColor = { 0, 0, 0, 255 };

bool state::closed = false;
bool state::running = true;


bool displayWaitingMessage = false;

SDL_Window* windowm = nullptr;
SDL_Renderer* rendererm = nullptr;



// Menu item structure
struct MenuItem {
	SDL_FRect rect;
	string label;
	bool isHovered;
	SDL_Texture* textTexture;
	int textWidth;
	int textHeight;
};


void initializeMenuWindowAndRenderer() {
	windowm = SDL_CreateWindow("Main Menu", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY);
	rendererm = SDL_CreateRenderer(windowm, NULL);

}

// Initialize menu items: Rect pos and size, text and initial isHovered value.
vector<MenuItem> menuItems = {
	{{(WINDOW_WIDTH - 400) / 2, 500, 400, 100}, "Start Game", false, nullptr, 0, 0}, // Start Game
	{{(WINDOW_WIDTH - 400) / 2, 650, 400, 100}, "Instructions", false, nullptr, 0, 0}, // Instructions
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
void renderMenu(SDL_Renderer* renderer, SDL_Texture* menuTexture, SDL_Texture* menuOptionTexture, SDL_Texture* menuOptionTextureS) {
	int textX = 0;
	SDL_RenderTexture(renderer, menuTexture, nullptr, nullptr);
	SDL_Color white = { 255, 255, 255, 255 };
	TextRender waitingText(renderer, "arial.ttf", 28);

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

	if (Client::displayWaitingMessage) {
		waitingText.updateText("Waiting for opponent...", white);
		textX = (WINDOW_WIDTH - waitingText.getTextWidth()) / 2;
		waitingText.renderText(textX, 450);
	}
}

// Check if mouse is over a rectangle
bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect) {
	return (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
		mouseY >= rect.y && mouseY <= rect.y + rect.h);
}


// Main menu loop
int runMenu(SDL_Window* window, SDL_Renderer* renderer) {
	SDL_Event event;
	int i = 0;

	SDL_Texture* menuTexture = LoadMenuTexture(rendererm);
	SDL_Texture* menuOptionTexture = LoadMenuOptionTexture(rendererm);
	SDL_Texture* menuOptionTextureS = LoadMenuOptionTextureSelected(rendererm);

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
	while (state::running) {
		//Client recieves the start permission from the server and starts the game
		if (Client::startperm == true) {
			cout << "Starting game..." << endl;
			state::running = false; // Close the menu after starting the game
		}
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				state::running = false;
				state::closed = true;
			}
			else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
				exit(EXIT_SUCCESS); // Close the window
			}
			else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
				// Get mouse coordinates
				float mouseX = static_cast<float>(event.motion.x);
				float mouseY = static_cast<float>(event.motion.y);

				// Check menu items
				for (size_t i = 0; i < menuItems.size(); i++) {
					menuItems[i].isHovered = isMouseOver(mouseX, mouseY, menuItems[i].rect);

					if (menuItems[i].isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
						Audio::PlaySoundFile("Assets/Sounds/ButtonClick.mp3");
						if (i == 2) {
							exit(EXIT_SUCCESS); // Exit
						}
						else if (i == 0) {
							// Start game
							std::thread notifyThread([&]() {
								std::cout << "Starting thread to notify server..." << std::endl;
								// Use discoverServer() if needed (or skip if already done)
								string server_ip = discoverServer();  // Optional: update the server IP
								Client::displayWaitingMessage = true;
								if (!Client::notifyStartGame()) {  // Call without parameters
									std::cerr << "Failed to notify server to start game session." << std::endl;
								}
								});
							notifyThread.detach();
						}
						else if (i == 1) {
							// Instructions
							MessageBox(NULL, L"Click and drag the blocks on the right to the grid and form lines.\n\nForming a line will clear all blocks in the line and award you with points.\n\nIf you get stuck then click the clear grid button to clear the entire grid.\n\nYou only have 3 minutes to get more points than your opponent!\n\nHave fun!", L"Instructions", MB_OK);

						}
					}
				}
			}
		}

		// Clear screen
		SDL_SetRenderDrawColor(rendererm, 255, 255, 255, 255);
		SDL_RenderClear(rendererm);

		// Render menu
		renderMenu(rendererm, menuTexture, menuOptionTexture, menuOptionTextureS);

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
