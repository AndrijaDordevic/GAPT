#include <iostream>
#include <SDL3/SDL.h>
#include "Window.hpp"  
#include "Grid.hpp"

/**
 * @brief Draws the entire game grid.
 *
 * This function creates a grid instance and calls its `Draw()` method.
 * The grid is centered based on the window dimensions.
 *
 * @param renderer SDL_Renderer instance used to draw the grid.
 */
void DrawGrid(SDL_Renderer* renderer) {  // Renamed from Draw -> DrawGrid
    int startX = (WINDOW_WIDTH - (Grid::COLUMNS * Grid::CELL_WIDTH)) / 2-100;
    int startY = OFFSET;

    Grid grid(startX, startY); // Create a Grid object
    grid.Draw(renderer); // Call the Grid's Draw() function to render it
}
