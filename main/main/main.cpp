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
#include "ScreenShake.hpp"
#include "windows.h"
#include <mutex>
#include <filesystem>
#include <iostream>
#include <direct.h>

using namespace std;
bool BGMRunning = false;
bool closeGame = false;

SDL_Window* gameWindow = nullptr;
SDL_Renderer* gameRenderer = nullptr;
ScreenShake shaker; 
SDL_FPoint cameraOffset = { 0, 0 };

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
    uint64_t last = SDL_GetTicksNS();


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

        uint64_t now = SDL_GetTicksNS();
        float deltaTime = (now - last) / 1'000'000'000.0f;
        last = now;


        cameraOffset = shaker.update(deltaTime);

        std::string displayTimer = Client::TimerBuffer.empty() ? "Ended" : Client::TimerBuffer;
        timerText.updateText(displayTimer, white);

        // Clear screen (background assumed transparent or set elsewhere)
        SDL_RenderClear(renderer);

        // Render game elements
        SDL_FRect bgRect = {
            -cameraOffset.x,
            -cameraOffset.y,
            static_cast<float>(WINDOW_WIDTH),
            static_cast<float>(WINDOW_HEIGHT)
        };
        SDL_RenderTexture(renderer, texture, nullptr, &bgRect);

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

void loadSnapshotAndRunGame()
{
    nlohmann::json snap;
    {
        std::lock_guard<std::mutex> lk(Client::snapMtx);
        snap = Client::lastSnapshot;
    }

    // restore score & queue index
    Client::myScore = snap["score"].get<int>();
    Client::nextShapeIdx = snap["nextShapeIdx"].get<size_t>();

    // rebuild the locked?block grid
    Client::lockedBlocks.clear();
    for (auto& c : snap["grid"]) {
        Client::lockedBlocks.push_back({ c["x"].get<int>(), c["y"].get<int>() });
    }

    // **new** — reload the next?shapes preview
    Client::shape.clear();
    if (snap.contains("upcomingShapes")) {
        for (auto& s : snap["upcomingShapes"]) {
            Client::shape.push_back(s.get<int>());
        }
    }

    // now start the normal game loop
    runGame(gameWindow, gameRenderer);
}

void printWorkingDir()
{
    char buf[_MAX_PATH];
    if (_getcwd(buf, _MAX_PATH)) {
        std::cout << "Working directory is: " << buf << '\n';
    }
    else {
        std::cout << "Could not query working directory\n";
    }
}

int main(int argc, char* argv[]) {
    // Run unit tests
    /*
    Test_validateHMAC_Missing();
    Test_validateHMAC_Correct();
    Test_validateHMAC_BadTag();
    Test_hmacEquals();
    Test_ComputeHMAC();
	Test_PairingSync();
    Test_stopBroadcast_flag();
	Test_resetClientState();
    Test_HoverStates();
    Test_InsideGrid();
    Test_VariableJitterSimulation();
    Test_PacketLossSimulation();
    */
    printWorkingDir();

 
    // Start client thread (remains detached throughout the application)
    thread clientThread(Client::runClient);
    clientThread.detach();

    if (Client::resumeNow.load()) {
        // clear the menu flag so runMenu() immediately returns “start”
        state::running = false;
        state::closed = false;
    }

    // Run the menu first.
    while (true) {
        if (Client::resumeNow.exchange(false)) {
            // user was reconnected—skip straight into the game:
            loadSnapshotAndRunGame();
            continue;
        }

        initializeMenuWindowAndRenderer();
        int menuSelection = runMenu(nullptr, nullptr);
        if (menuSelection == 0) {
            Client::notifyStartGame();
            runGame(gameWindow, gameRenderer);
            if (closeGame) break;
        }
        else if (menuSelection == 2) {
            break;
        }
    }
    Client::shutdownConnection();
    SDL_Quit();
    return 0;
}
