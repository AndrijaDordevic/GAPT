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
    bool isPressed;
};

vector<ClearGridButton> clrButtons = {
    {{(WINDOW_WIDTH - 400) / 2, 250, 400, 100}, "Clear Screen", false, false}
};

bool CheckHover(float mouseX, float mouseY, const SDL_FRect& rect) {
    return (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
        mouseY >= rect.y && mouseY <= rect.y + rect.h);
}

void renderClear(SDL_Renderer* renderer, const ClearGridButton& button) {
    SDL_RenderFillRect(renderer, &button.rect);

    SDL_RenderRect(renderer, &button.rect);

}

void clearTetrominos(vector<vector<int>>& grid) {
    // Clear the grid
    for (auto& row : grid) {
        fill(row.begin(), row.end(), 0);
    }
    // Clear the active and placed tetrominos
    tetrominos.clear();
    placedTetrominos.clear();
    cout << "Grid and tetrominos cleared!" << endl;
}

void handleClearButtonEvent(SDL_Event& event, vector<vector<int>>& grid) {
    float mouseX = static_cast<float>(event.motion.x);
    float mouseY = static_cast<float>(event.motion.y);

    for (auto& button : clrButtons) {
        button.isHovered = CheckHover(mouseX, mouseY, button.rect);

        if (button.isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            button.isPressed = true;
        }
        else if (button.isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_UP && button.isPressed) {
            clearTetrominos(grid);
            button.isPressed = false;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            button.isPressed = false;
        }
    }
}

int ClearScreenButton(SDL_Renderer* renderer, vector<vector<int>>& grid) {
    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                handleClearButtonEvent(event, grid);
            }
        }

        // Render the button
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        for (const auto& button : clrButtons) {
            renderClear(renderer, button);
        }
        SDL_RenderPresent(renderer);
    }
    return 0;
}