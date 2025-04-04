#include "Window.hpp"  
#include "Menu.hpp"
#include "Tetromino.hpp"
#include "DragBlock.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "Client.hpp"
#include "Texture.hpp"

#include "Timer.hpp"
#include "TextRender.hpp"
#include <thread>

using namespace std;


int main() {
	bool running = true;
	SDL_Event event;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;

	// Connect client
	thread clientThread(runClient);
	clientThread.detach(); // Detach the thread to keep running

	// Run menu 
	runMenu(window, renderer);

	if (closed == false) {
		if (!initializeSDL(window, renderer)) {
			cerr << "Failed to initialize SDL." << endl;
			return 1;
		}
		srand(static_cast<unsigned>(time(0)));

		// Load game background texture
		texture = LoadGameTexture(renderer);

		if (!LoadBlockTextures(renderer)) {
			cerr << "Failed to load textures!\n";
			return -1;
		}

		// Load font and create TextRender object
		SDL_Color white = { 255, 255, 255, 255 };
		TextRender timerText(renderer, "arial.ttf", 28);

		// Main game loop
		while (running) {
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EVENT_QUIT) {
					running = false;
				}
				DragDrop(event); // Handle drag events
			}

			// If the client is no longer running, stop the game
			if (!client_running) {
				cout << "Client disconnected, closing game..." << endl;
				running = false;
			}

			string timeStr = UpdateTime();
			timerText.updateText(timeStr, white);
			GameOverCheck();

			// Clear screen (but don't set a background color)
			SDL_RenderClear(renderer);

			// Render game elements
			SDL_RenderTexture(renderer, texture, NULL, NULL);
			RunBlocks(renderer);
			RenderTetrominos(renderer);

			// Render text (overlay on top)
			timerText.renderText(750, 47);

			// Update screen
			SDL_RenderPresent(renderer);
			SDL_Delay(16);
		}

		cleanupSDL(window, renderer);
	}

	return 0;
}