#include <iostream>
#include <SDL3/SDL.h>
#include "Window.hpp"  // Includes the function declaration
#include "Grid.hpp"


// Modified DrawGrid loop:
void DrawGrid(SDL_Renderer* renderer) {
   

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Draw grid lines
    int startX = (WINDOW_WIDTH - (GRID_COLUMNS * CELL_WIDTH)) / 2 - 100;
    int startY = OFFSET;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);


    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black lines
    // Draw vertical grid lines
    for (int i = 0; i <= GRID_COLUMNS; ++i) {
        int x = startX + i * CELL_WIDTH;
        SDL_RenderLine(renderer, x, startY, x, startY + (GRID_ROWS * CELL_HEIGHT));
    }

    // Draw horizontal grid lines
    for (int i = 0; i <= GRID_ROWS; ++i) {
         int y = startY + i * CELL_HEIGHT;
         SDL_RenderLine(renderer, startX, y, startX + (GRID_COLUMNS * CELL_WIDTH), y);
    }

    
}




