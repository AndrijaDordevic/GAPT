#include "Window.hpp"  // Includes drawGrid() declaration
#include "Menu.hpp"
#include "Blocks.hpp"  
#include "Grid.hpp"


int main() {
    bool running = true;
    SDL_Event event;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    runMenu(window, renderer);

    if (!initializeSDL(window, renderer)) {
        cerr << "Failed to initialize SDL." << endl;
        return 1;
    }
    srand(static_cast<unsigned>(time(0)));
        
    

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            DragDrop(event); // Handle events here
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Call DrawGrid() to draw a grid
        DrawGrid(renderer);
        // Update game state (spawn tetrominos)
        RunBlocks(renderer);

        // Render all tetrominos every frame
        RenderTetrominos(renderer);

        // Update screen
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    cleanupSDL(window, renderer);
    return 0;
}

