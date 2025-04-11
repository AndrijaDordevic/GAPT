#ifndef CLEAR_GRID_HPP
#define CLEAR_GRID_HPP

#include <SDL3/SDL.h>
#include <iostream>
#include <vector>

int runClearGridButton(SDL_Renderer* renderer);

void clearTetrominos();
void updateClearGridButton(const SDL_Event& event);



#endif // CLEAR_GRID_HPP
