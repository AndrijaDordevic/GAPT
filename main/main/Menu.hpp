#ifndef MENU_H
#define MENU_H

extern bool closed;

// Function declaration
int runMenu(SDL_Window* window, SDL_Renderer* renderer);

bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect);

void renderMenu(SDL_Renderer* renderer);

bool loadFontTexture();

void cleanupTextures();

#endif
