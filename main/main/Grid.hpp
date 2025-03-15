#ifndef GRID_HPP
#define GRID_HPP

#include <SDL3/SDL.h>

/**
 * @brief Represents a grid of cells.
 * Draws grid lines instead of individual cell borders to prevent overlap.
 */
class Grid {
public:
    static const int COLUMNS = 10;
    static const int ROWS = 10;
    static const int CELL_WIDTH = 80;
    static const int CELL_HEIGHT = 80;

private:
    int startX, startY; // Top-left corner of the grid

public:
    /**
     * @brief Constructs the grid at a specific position.
     * @param startX X-coordinate of the grid's top-left corner
     * @param startY Y-coordinate of the grid's top-left corner
     */
    Grid(int startX, int startY) : startX(startX), startY(startY) {}

    /**
     * @brief Draws the grid lines without overlapping borders.
     * @param renderer SDL_Renderer instance used for drawing.
     */
    void Draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black lines

        // Draw horizontal grid lines
        for (int i = 0; i <= ROWS; ++i) {
            const float y = startY + i * CELL_HEIGHT;
            SDL_RenderLine(renderer,
                startX, y,
                startX + COLUMNS * CELL_WIDTH, y
            );
        }

        // Draw vertical grid lines
        for (int j = 0; j <= COLUMNS; ++j) {
            const float x = startX + j * CELL_WIDTH;
            SDL_RenderLine(renderer,
                x, startY,
                x, startY + ROWS * CELL_HEIGHT
            );
        }
    }
};

#endif // GRID_HPP