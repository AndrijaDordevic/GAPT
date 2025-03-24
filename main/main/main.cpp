#include "Window.hpp"  // Includes drawGrid() declaration
#include "Menu.hpp"
#include "Grid.hpp"
#include "Tetromino.hpp"
#include "DragBlock.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>







int main() {
    bool running = true;
    SDL_Event event;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    runMenu(window, renderer);




    if (closed == false) {

        if (!initializeSDL(window, renderer)) {
            cerr << "Failed to initialize SDL." << endl;
            return 1;
        }
        srand(static_cast<unsigned>(time(0)));

        // Load image
        const char* path = "Assets/GameUI.png";

        SDL_Texture* texture = IMG_LoadTexture(renderer, path);
        if (!texture) {
            printf("CreateTextureFromSurface failed: %s\n", SDL_GetError());
        }


        if (!LoadBlockTextures(renderer)) {
            std::cerr << "Failed to load textures!\n";
            return -1;
        }



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

                SDL_RenderTexture(renderer, texture, NULL, NULL);

                // Call DrawGrid() to draw a grid
                //DrawGrid(renderer);
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
    }


