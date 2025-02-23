#define SDL_MAIN_USE_CALLBACKS 1  // Use callbacks instead of the main() method
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 900;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static TTF_Font* font = NULL;

// This function runs once on startup
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_Color colour = { 255, 255, 255, SDL_ALPHA_OPAQUE };
    SDL_Surface* text;

    // Create window
    if (!SDL_CreateWindowAndRenderer("Main Menu", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Open the font
    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create text
    text = TTF_RenderText_Blended(font, "Start Game\nSettings\nExit Game", 0, colour);
    if (text) {
        texture = SDL_CreateTextureFromSurface(renderer, text);
        SDL_DestroySurface(text);
    }
    if (!texture) {
        SDL_Log("Couldn't create text: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

// Fn for keypresses
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    // Pressing any key or clicking will end the program
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_QUIT || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        return SDL_APP_SUCCESS;  // end the program and report to os
    }
    return SDL_APP_CONTINUE;
}

// This runs every frame
SDL_AppResult SDL_AppIterate(void* appstate)
{
    int w = 0, h = 0;
    SDL_FRect dst;
    const float scale = 4.0f;

    // Center text and scale it up
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_GetTextureSize(texture, &dst.w, &dst.h);
    dst.x = ((w / scale) - dst.w) / 2;
    dst.y = ((h / scale) - dst.h) / 2;

    // Draw text
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, &dst);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

// Shutdown fn
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
}
