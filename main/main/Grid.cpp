#include <iostream>
#include <SDL3/SDL.h>
#include "Window.hpp"  // Includes the function declaration

void drawGrid(SDL_Renderer* renderer) {
    int grid[GRID_ROWS][GRID_COLUMNS] = { 0 }; // Initialize grid with all zeros

    // Calculate the total width and height for the grid
    int totalGridWidth = GRID_COLUMNS * CELL_WIDTH;
    int totalGridHeight = GRID_ROWS * CELL_HEIGHT;

    // Calculate the starting positions for centering the grid
    int startX = (WINDOW_WIDTH - totalGridWidth - 300) / 2;
    int startY = (WINDOW_HEIGHT - totalGridHeight) / 2;

    // Set background color to white
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Set grid line color to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Draw vertical grid lines
    for (int i = 0; i <= GRID_COLUMNS; i++) {
        int x = startX + i * CELL_WIDTH;
        SDL_RenderLine(renderer, x, startY, x, startY + totalGridHeight);
    }

    // Draw horizontal grid lines
    for (int i = 0; i <= GRID_ROWS; i++) {
        int y = startY + i * CELL_HEIGHT;
        SDL_RenderLine(renderer, startX, y, startX + totalGridWidth, y);
    }

    // Render the updated grid
    SDL_RenderPresent(renderer);
}
