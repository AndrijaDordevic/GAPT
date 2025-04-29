#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include "ClearGrid.hpp"
#include "Window.hpp"
#include "Menu.hpp"
#include "Tetromino.hpp"
#include "Texture.hpp"
#include "Audio.hpp"
#include "ScreenShake.hpp"

using namespace std;
extern ScreenShake shaker;
struct ClearGridButton {
    SDL_FRect rect;
    string clrlabel;
    bool isHovered;

};

vector<ClearGridButton> clrButton = {
    {{(WINDOW_WIDTH + 173) / 2, 20, 130, 130}, "Clear Screen", false}
};


void clearTetrominos() {
    placedTetrominos.clear();
    Audio::PlaySoundFile("Assets/Sounds/ClearGrid.mp3");
    shaker.start(0.6f, 12.0f);
}


extern SDL_FPoint cameraOffset;  // ← ADD this at top if not already

int runClearGridButton(SDL_Renderer* renderer, SDL_Texture* ClearGridSelect, SDL_Texture* ClearGridSelectS) {
    for (auto& button : clrButton) {
        SDL_FRect offsetRect = {
            button.rect.x - cameraOffset.x,
            button.rect.y - cameraOffset.y,
            button.rect.w,
            button.rect.h
        };

        button.isHovered
            ? SDL_RenderTexture(renderer, ClearGridSelectS, nullptr, &offsetRect)
            : SDL_RenderTexture(renderer, ClearGridSelect, nullptr, &offsetRect);
    }
    return 0;
}


        

    



// Function to update the button state based on mouse events.
void updateClearGridButton(const SDL_Event& event) {
     
    if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        // Get mouse coords as float
        float mouseX = static_cast<float>(event.motion.x);
        float mouseY = static_cast<float>(event.motion.y);

        for (auto& button : clrButton) {
            button.isHovered = isMouseOver(mouseX, mouseY, button.rect);
            

                if (button.isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                // Clear the tetrominos when clicked.
                clearTetrominos();
            }
        }
    }
}
            


