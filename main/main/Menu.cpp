#include "Window.hpp"
#include "Menu.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>

using namespace std;

// Menu item structure
struct MenuItem {
    SDL_FRect rect;
    string label;
    bool isHovered;
};


// Create window 
SDL_Window* window = SDL_CreateWindow("Main Menu", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

// Create renderer for window
SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

// Initialize menu items: Rect size and pos, text and initial isHovered value.
vector<MenuItem> menuItems = {
    {{300, 200, 200, 50}, "Start Game", false}, // Start Game
    {{300, 270, 200, 50}, "Options", false}, // Settings
    {{300, 340, 200, 50}, "Exit", false}  // Exit
};

// Function to render menu
void renderMenu(SDL_Renderer* renderer) {
    for (const auto& item : menuItems) {
        SDL_Color color = item.isHovered ? SDL_Color{ 0, 255, 0, 255 } : SDL_Color{ 255, 0, 0, 255 };
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &item.rect);
    }
}

// Function to check if mouse is within a rectangle
bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect) {
    return (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
        mouseY >= rect.y && mouseY <= rect.y + rect.h);
}

// This fn 
int runMenu(SDL_Window* window, SDL_Renderer* renderer) {
    bool running = true;
    SDL_Event event;

    //  While the window is open, this handles events like mouse motion and clicks.
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
                        else cout << "Menu Item " << i + 1 << " clicked!\n";
                    }
                }
            }
        }
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Render menu
        renderMenu(renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
