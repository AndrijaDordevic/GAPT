#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h> // Need additional library to render text
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 900;

// Menu item structure
struct MenuItem {
    SDL_Rect rect;
    std::string label;
    bool isHovered;
};

// Function to render text
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,size_t size, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), size, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

int main() {

    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0) {
        std::cerr << "SDL/TTF Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Main Menu", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!window || !renderer) {
        std::cerr << "Window/Renderer Creation Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);

    // Define menu items
    std::vector<MenuItem> menuItems = {
        {{300, 200, 200, 50}, "Start", false},
        {{300, 270, 200, 50}, "Options", false},
        {{300, 340, 200, 50}, "Exit", false}
    };

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int mouseX = event.motion.x;
                int mouseY = event.motion.y;

                for (auto& item : menuItems) {
                    item.isHovered = SDL_PointInRect(&(SDL_Point) {mouseX, mouseY}, & item.rect);

                    if (item.isHovered && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (item.label == "Exit") running = false;
                        else std::cout << item.label << " clicked!\n";
                    }
                }
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw menu
        for (auto& item : menuItems) {
            SDL_Color color = item.isHovered ? SDL_Color{ 255, 0, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
            SDL_Texture* textTexture = renderText(renderer, font, item.label, size, color);
            SDL_RenderTexture(renderer, textTexture, NULL, &item.rect);
            SDL_DestroyTexture(textTexture);
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
