#include "Window.hpp"
#include "Menu.hpp"
#include "Tetromino.hpp"
#include "DragBlock.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "Client.hpp"
#include "TextRender.hpp"
#include <thread>
#include <iostream>
#include <cstdlib>
#include "Texture.hpp"

using namespace std;

// Function that runs the game loop.
void runGame(SDL_Window* window, SDL_Renderer* renderer) {
    if (!initializeSDL(window, renderer)) {
        cerr << "Failed to initialize SDL." << endl;
        exit(1);
    }
    srand(static_cast<unsigned>(time(0)));

    // Load game background texture and block textures
    SDL_Texture* texture = LoadGameTexture(renderer);
    if (!LoadBlockTextures(renderer)) {
        cerr << "Failed to load textures!" << endl;
        exit(-1);
    }

    // Create text rendering object for timer
    SDL_Color white = { 255, 255, 255, 255 };
    TextRender timerText(renderer, "arial.ttf", 28);

    bool running = true;
    SDL_Event event;

    // Main game loop
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            DragDrop(event); // Handle drag events
        }

        // Check client connection status
        if (!Client::client_running) {
            cout << "Client disconnected, closing game..." << endl;
            running = false;
        }
        std::string displayTimer = Client::TimerBuffer.empty() ? "Starting" : Client::TimerBuffer;
        timerText.updateText(displayTimer, white);

        // Clear screen (background assumed transparent or set elsewhere)
        SDL_RenderClear(renderer);

        // Render game elements
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        RunBlocks(renderer);
        RenderScore(renderer, score);
        RenderTetrominos(renderer);

        // Render timer text
        timerText.renderText(735, 47);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    cleanupSDL(window, renderer);
}

int main() {
    // Start client thread (remains detached throughout the application)
    thread clientThread(Client::runClient);
    clientThread.detach();

    // Run the menu first. 
    // The runMenu function could return an int or enum indicating the selected menu option.
    int menuSelection = runMenu(nullptr, nullptr);

    // If the user selected "Start Game", start the game.
    if (menuSelection == 0) {
        // Create SDL window and renderer for the game.
        SDL_Window* gameWindow = nullptr;
        SDL_Renderer* gameRenderer = nullptr;
        runGame(gameWindow, gameRenderer);
    }

    return 0;
}
