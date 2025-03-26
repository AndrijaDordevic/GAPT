#include "Window.hpp"  // Includes drawGrid() declaration
#include "Menu.hpp"
#include "Grid.hpp"
#include "Tetromino.hpp"
#include "DragBlock.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "Client.hpp"
#include "Texture.hpp"
#include "Listener.hpp"
#include <thread>






int main() {
    bool running = true;
    SDL_Event event;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    // Start the listener in a separate thread to capture the server's IP
    std::thread listenerThread(listenForServerIP);
    listenerThread.detach();  // Let it run in the background

    // Run menu and handle the client's thread (start it when "Start Game" is clicked)
    runMenu(window, renderer);

    if (closed == false) {
        if (!initializeSDL(window, renderer)) {
            std::cerr << "Failed to initialize SDL." << std::endl;
            return 1;
        }
        srand(static_cast<unsigned>(time(0)));

        // Load image
        texture = LoadGameTexture(renderer);

        if (!LoadBlockTextures(renderer)) {
            std::cerr << "Failed to load textures!\n";
            return -1;
        }

        // Main game loop
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                DragDrop(event); // Handle events here
            }

            // If the client is no longer running, stop the game
            if (!client_running) {
                std::cout << "Client disconnected, closing game..." << std::endl;
                running = false;
            }

            // Clear screen
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, NULL);

            RunBlocks(renderer);
            RenderTetrominos(renderer);

            // Update screen
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
        }

        cleanupSDL(window, renderer);
    }

    return 0;
}


