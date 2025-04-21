#include "Window.hpp"
#include "Menu.hpp"
#include "Tetromino.hpp"
#include "DragBlock.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "Client.hpp"
#include "TextRender.hpp"
#include "ClearGrid.hpp"
#include <thread>
#include <iostream>
#include <cstdlib>
#include "Texture.hpp"
#include "Audio.hpp"
#include "UnitTests.hpp"
#include "windows.h"

using namespace std;
bool BGMRunning = false;
bool closeGame = false;

SDL_Window* gameWindow = nullptr;
SDL_Renderer* gameRenderer = nullptr;

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
    SDL_Texture* ClearGridSelect = LoadClearGridTexture(renderer);
    SDL_Texture* ClearGridSelectS = LoadClearGridTextureS(renderer);

    bool running = true;
    SDL_Event event;

    Audio::Init();
    // Main game loop
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                if(Client::inSession){
                    closeGame = true;
                }
            }
            DragDrop(event); // Handle drag events
            updateClearGridButton(event);

        }

        // Check client connection status
        if (Client::gameOver) {
            cout << "Game over, closing game..." << endl;
            running = false;
        }
        std::string displayTimer = Client::TimerBuffer.empty() ? "Ended" : Client::TimerBuffer;
        timerText.updateText(displayTimer, white);

        // Clear screen (background assumed transparent or set elsewhere)
        SDL_RenderClear(renderer);

        // Render game elements
        SDL_RenderTexture(renderer, texture, NULL, NULL);


		score = Client::UpdateScore();
        RunBlocks(renderer);
        RenderScore(renderer, score, OpponentScore);
        runClearGridButton(renderer, ClearGridSelect,ClearGridSelectS);
        RenderTetrominos(renderer);



        // Render timer text
        timerText.renderText(735, 47);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
        if (!BGMRunning) {
            Audio::PlaySoundFile("Assets/Sounds/BackgroundMusic.mp3");
            BGMRunning = true;
        }

    }
    //Audio::Shutdown();
    cleanupSDL(window, renderer);
}


int main(int argc, char* argv[]) {

    // Start client thread (remains detached throughout the application)
    Test_HoverStates();
    Test_InsideGrid(); // Run unit tests
    thread clientThread(Client::runClient);
    clientThread.detach();

    // Run the menu first.
    while (true) {
        initializeMenuWindowAndRenderer();
        int menuSelection = runMenu(nullptr, nullptr);
        if (menuSelection == 0) {
            if (!initializeSDL(gameWindow, gameRenderer)) {
                std::cerr << "Failed to initialize SDL. Exiting..." << std::endl;
                return -1;
            }
            runGame(gameWindow, gameRenderer);
            if (closeGame) {
                break;
            }
        }
        else if (menuSelection == 2) {
            return 0;
        }
    }

    SDL_Quit();
    return 0;
}
