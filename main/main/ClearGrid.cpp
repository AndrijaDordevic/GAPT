#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include "ClearGrid.hpp"
#include "Window.hpp"
#include "Menu.hpp"
#include "Tetromino.hpp"
#include "Texture.hpp"

using namespace std;

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
}


int runClearGridButton(SDL_Renderer* renderer) {
    SDL_Event event;
    bool running = true;
    SDL_Texture* ClearGridSelect = LoadClearGridTexture(renderer);
    SDL_Texture* ClearGridSelectS = LoadClearGridTextureS(renderer);


                for (auto& button : clrButton) {
                    

                    button.isHovered ? SDL_RenderTexture(renderer, ClearGridSelectS, nullptr, &button.rect) : SDL_RenderTexture(renderer, ClearGridSelect, nullptr, &button.rect);
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
            


