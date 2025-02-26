#include "Window.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>


// Menu item structure
struct MenuItem {
    SDL_FRect rect;
    std::string label;
    bool isHovered;
};

// Initialize menu items
std::vector<MenuItem> menuItems = {
    {{300, 200, 200, 50}, "Start Game", false}, // Start Game
    {{300, 270, 200, 50}, "Options", false}, // Settings
    {{300, 340, 200, 50}, "Exit", false}  // Exit
};

// Function to render menu
void renderMenu(SDL_Renderer* renderer) {
    for (size_t i = 0; i < menuItems.size(); i++) {
        SDL_Color color = menuItems[i].isHovered ? SDL_Color{ 255, 0, 0, 255 } : SDL_Color{ 0, 0, 255, 255 };
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &menuItems[i].rect);
    }
}

// Function to check if mouse is within a rectangle
bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect) {
    return (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
        mouseY >= rect.y && mouseY <= rect.y + rect.h);
}

int main(int argc, char* argv[]) {
    // Initialise SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << "\n";
        return 1;
    }
    // Create window 
    SDL_Window* window = SDL_CreateWindow("Menu", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    // Create renderer for window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!window || !renderer) {
        std::cerr << "Window/Renderer Creation Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                // Get mouse coords as flt
                float mouseX = static_cast<float>(event.motion.x);
                float mouseY = static_cast<float>(event.motion.y);

                // Loop through menu items to check hover and clicks
                for (size_t i = 0; i < menuItems.size(); i++) {
                    // Check if mouse is over an item in the menu
                    menuItems[i].isHovered = isMouseOver(mouseX, mouseY, menuItems[i].rect);

                    if (menuItems[i].isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (i == 2) running = false; // Exit menu if exit is clicked
                        else std::cout << "Menu Item " << i + 1 << " clicked!\n";
                    }
                }
            }
        }
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw menu
        renderMenu(renderer);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}