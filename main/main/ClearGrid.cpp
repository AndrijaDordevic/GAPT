#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include "ClearGrid.hpp"
#include "Window.hpp"
#include "Menu.hpp"
#include "Tetromino.hpp"

using namespace std;

struct ClearGridButton {
    SDL_FRect rect;
    string clrlabel;
    bool isHovered;
};

vector<ClearGridButton> clrButton = {
    {{(WINDOW_WIDTH + 210) / 2, 40, 90, 90}, "Clear Screen", false}
};

void renderButton(SDL_Renderer* renderer) {
    for (const auto& button : clrButton) {
        SDL_Color clrcolour = button.isHovered ? SDL_Color{ 255, 0, 0, 0 } : SDL_Color{ 0, 0, 255, 0 };
        SDL_SetRenderDrawColor(renderer, clrcolour.r, clrcolour.g, clrcolour.b, clrcolour.a);
        SDL_RenderFillRect(renderer, &button.rect);
    }
}

void clearTetrominos() {
    placedTetrominos.clear();
}


int runClearGridButton(SDL_Renderer* renderer) {
    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                // Get mouse coords as flt
                float mouseX = static_cast<float>(event.motion.x);
                float mouseY = static_cast<float>(event.motion.y);

                for (auto& button : clrButton) {
                    button.isHovered = isMouseOver(mouseX, mouseY, button.rect);
                    if (button.isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        // Clear the screen
                        clearTetrominos();
                    }
                }

            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
        // Render the button
        for (const auto& button : clrButton) {
            renderButton(renderer);
        }
        renderButton(renderer);
        SDL_RenderPresent(renderer);
    }
    return 0;
}
