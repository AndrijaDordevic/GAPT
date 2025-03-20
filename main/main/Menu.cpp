#include "Window.hpp"
#include "Menu.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <SDL3_ttf/SDL_ttf.h>


using namespace std;

TTF_Font* font = nullptr;
SDL_Color textColor = { 0, 0, 0, 255 };

bool closed = false;

// Menu item structure
struct MenuItem {
    SDL_FRect rect;
    string label;
    bool isHovered;
    SDL_Texture* textTexture;
	int textWidth;
	int textHeight;
};
bool running = true;

// Create window 
SDL_Window* windowm = SDL_CreateWindow("Main Menu", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

// Create renderer for window
SDL_Renderer* rendererm = SDL_CreateRenderer(windowm, NULL);


// Initialize menu items: Rect pos and size, text and initial isHovered value.
vector<MenuItem> menuItems = {
    {{(WINDOW_WIDTH - 400 )/2, 250, 400, 100}, "Start Game", false, nullptr, 0, 0}, // Start Game
    {{(WINDOW_WIDTH - 400)/2, 400, 400, 100}, "Options", false, nullptr, 0, 0}, // Options
    {{(WINDOW_WIDTH - 400)/2, 550, 400, 100}, "Exit", false, nullptr, 0, 0}  // Exit
};

bool loadFontTexture() {

    TTF_Font* font = TTF_OpenFont("C:\\Arial.ttf", 24);

	if (!font) {
		cerr << "Failed to load font." << SDL_GetError << endl;
		return false;
	}

    for (auto& item : menuItems) {

        SDL_Surface* textSurface = TTF_RenderText_Blended(font, item.label.c_str(), 10, textColor);
        if (!textSurface) {
            cerr << "Failed to create text surface." << SDL_GetError() << endl;
            return false;
        }

		item.textTexture = SDL_CreateTextureFromSurface(rendererm, textSurface);
		item.textWidth = textSurface->w;
		item.textHeight = textSurface->h;
		SDL_DestroySurface(textSurface);

		if (!item.textTexture) {
			cerr << "Failed to create text texture." << SDL_GetError() << endl;
			return false;
		}

    }
	return true;

}

void cleanupTextures() {
	for (auto& item : menuItems) {
		if (item.textTexture) {
			SDL_DestroyTexture(item.textTexture);
			item.textTexture = nullptr;
		}
	}
}   

// Function to render menu
void renderMenu(SDL_Renderer* renderer) {
    for (const auto& item : menuItems) {
        SDL_Color color = item.isHovered ? SDL_Color{ 0, 255, 0, 255 } : SDL_Color{ 255, 0, 0, 255 };
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &item.rect);

        if (item.textTexture) {
            SDL_FRect textRect = {
                item.rect.x + (item.rect.w - item.textWidth) / 2,
                item.rect.y + (item.rect.h - item.textHeight) / 2,
                static_cast<float>(item.textWidth),
                static_cast<float>(item.textHeight)
            };
            SDL_RenderTexture(renderer, item.textTexture, nullptr, &textRect);
        }
    }
}

// Function to check if mouse is within a rectangle
bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect) {
    return (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
        mouseY >= rect.y && mouseY <= rect.y + rect.h);
}



// This fn 
int runMenu(SDL_Window* window, SDL_Renderer* renderer) {

    SDL_Event event;

	//if (!loadFontTexture()) {
	//	cerr << "Failed to load font texture." << SDL_GetError() << endl;
	//	return 1;
	//}

	if (TTF_Init() == false) {
		cerr << "Failed to initialize TTF." << SDL_GetError() << endl;
		return 1;
	}

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
                        if (i == 2) exit(EXIT_SUCCESS); // Exit menu if exit is clicked

                        else if (i == 0) {
                            // Start game
                            running = false;

                        }
                        else if (i == 1) {
                            // Options
                            cout << "Options..." << endl;
                        }
                    }
                }
            }
        }
        // Clear screen
        SDL_SetRenderDrawColor(rendererm, 255, 255, 255, 255);
        SDL_RenderClear(rendererm);

        // Render menu
        renderMenu(rendererm);
        SDL_RenderPresent(rendererm);
        SDL_Delay(16);
    }

    // Cleanup
	cleanupTextures();
    TTF_CloseFont(font);
	TTF_Quit();
    SDL_DestroyRenderer(rendererm);
    SDL_DestroyWindow(windowm);
    SDL_Quit();
    return 0;
}
