#include "Window.hpp" 


bool initializeSDL(SDL_Window*& window, SDL_Renderer*& renderer) {

	// Create SDL Window with specified title, width, height, and resizable property
	window = SDL_CreateWindow("Block Game", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY);

	// Check if window creation failed
	if (!window) {
		cerr << "Failed to create window: " << SDL_GetError() << endl; // Print SDL error message
		return false; // Return failure
	}

	// Create an SDL Renderer linked to the created window
	renderer = SDL_CreateRenderer(window, NULL);

	// Check if renderer creation failed
	if (!renderer) {
		cerr << "Failed to create renderer: " << SDL_GetError() << endl; // Print SDL error message
		SDL_DestroyWindow(window); // Destroy the window to free memory before exiting
		return false; // Return failure
	}

	return true; // Return success if both window and renderer are created successfully
}

void handleEvents(bool& running) {
	SDL_Event event; // SDL event structure to store event information

	// Poll events from the event queue
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) { // If the user clicks 'X' or presses ALT+F4
			running = false; // Set running to false to exit the main loop
		}
	}
}


void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer) {
	SDL_DestroyRenderer(renderer); // Destroy the renderer to free GPU resources
	SDL_DestroyWindow(window); // Destroy the window to free memory
	SDL_Quit(); // Quit SDL and clean up all initialized subsystems
}

