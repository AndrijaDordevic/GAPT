#include "Window.hpp"  // Includes drawGrid() declaration

int main() {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!initializeSDL(window, renderer)) {
        return -1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        handleEvents(running);  // Handle user events

        drawGrid(renderer);  // Call grid to drawGrid to each frame

        SDL_Delay(100);  // Control the frame rate
    }

    cleanupSDL(window, renderer);
    return 0;
}
