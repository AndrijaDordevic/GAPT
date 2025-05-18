#ifndef MENU_H
#define MENU_H

namespace Client {
	extern bool displayWaitingMessage;
}

namespace state {
	extern bool closed;
	extern bool running;
}

int runMenu(SDL_Window* window, SDL_Renderer* renderer);

bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect);

void renderMenu(SDL_Renderer* renderer, SDL_Texture* menuTexture, SDL_Texture* menuOptionTexture, SDL_Texture* menuOptionTextureS);

bool loadFontTexture();

void cleanupTextures();

void initializeMenuWindowAndRenderer();

#endif
