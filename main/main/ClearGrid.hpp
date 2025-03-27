#ifndef CLEAR_GRID_HPP
#define CLEAR_GRID_HPP

#include <SDL3/SDL.h>
#include <vector>

bool CheckHover(float mouseX, float mouseY, const SDL_FRect& rect);
void renderClear(SDL_Renderer* renderer);
int ClearScreenButton();


#endif // CLEAR_GRID_HPP
