#include "WindowManager.hpp"
#include <SDL3/SDL.h>

// Define the global variable
WindowConfig windowConfig = { 800, 600 }; // Default size

WindowManager::WindowManager(const char* title, int width, int height)
    : window(nullptr), renderer(nullptr), running(true)
{
    windowConfig.width = width;
    windowConfig.height = height;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        running = false;
        return;
    }

    window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        running = false;
        return;
    }

    // Create a renderer with default settings (hardware-accelerated by default)
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        running = false;
        return;
    }
}

WindowManager::~WindowManager()
{
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

bool WindowManager::HandleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_KEY_DOWN) {
            running = false;
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            SDL_GetWindowSize(window, &windowConfig.width, &windowConfig.height);
            SDL_Log("Window resized to: %d x %d", windowConfig.width, windowConfig.height);
        }
    }
    return running;
}

void WindowManager::Clear()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void WindowManager::Present()
{
    SDL_RenderPresent(renderer);
}

bool WindowManager::IsRunning() const
{
    return running;
}