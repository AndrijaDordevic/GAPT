#ifndef CLEAR_GRID_HPP
#define CLEAR_GRID_HPP

#include <SDL3/SDL.h>
#include <iostream>
#include <vector>

int runClearGridButton(SDL_Renderer* renderer, SDL_Texture* ClearGridSelect, SDL_Texture* ClearGridSelectS);

void clearTetrominos();
void updateClearGridButton(const SDL_Event& event);

#endif 
