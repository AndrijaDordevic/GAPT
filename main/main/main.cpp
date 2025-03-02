#include "Window.hpp"  // Includes drawGrid() declaration
#include "Blocks.hpp"
#include "Menu.hpp"


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

        runMenu(); // Create the menu

        RunBlocks(window, renderer, WINDOW_WIDTH, WINDOW_HEIGHT); //Spawn Blocks
    }

    cleanupSDL(window, renderer);
    return 0;
}
