#ifndef CELL_HPP
#define CELL_HPP

#include <SDL3/SDL.h>

/**
 * @brief Represents a single cell in the grid.
 *
 * Each cell has an (x, y) position and dimensions (width, height).
 * The cell can be drawn with an outline.
 */
struct Cell {
    float x, y, width, height;  // Position and size of the cell

    /**
     * @brief Constructs a cell with given position and size.
     * @param x X-coordinate of the top-left corner
     * @param y Y-coordinate of the top-left corner
     * @param width Width of the cell
     * @param height Height of the cell
     */
    Cell(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height) {
    }

    /**
     * @brief Draws the cell outline on the screen.
     * @param renderer SDL_Renderer instance used to draw the cell.
     */
    void Draw(SDL_Renderer* renderer) const {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set color to black
        SDL_FRect rect = { x, y, width, height };  // Use SDL_FRect for floating point precision
        SDL_RenderRect(renderer, &rect);  // Draw the rectangle outline
    }
};

#endif // CELL_HPP
