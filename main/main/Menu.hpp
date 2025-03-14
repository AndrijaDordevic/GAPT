#ifndef MENU_H
#define MENU_H

int runMenu(SDL_Window* window, SDL_Renderer* renderer);
bool isMouseOver(float mouseX, float mouseY, const SDL_FRect& rect);
void renderMenu(SDL_Renderer* renderer);


#endif