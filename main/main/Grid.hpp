#ifndef GRID_HPP
#define GRID_HPP

#include <vector>
#include "Cell.hpp"

/**
 * @brief Represents a grid of cells.
 *
 * The grid consists of multiple rows and columns of `Cell` objects.
 * Each cell has a fixed width and height.
 */
class Grid {
public:
    // Grid dimensions
    static const int COLUMNS = 10;  // Number of columns in the grid
    static const int ROWS = 10;     // Number of rows in the grid
    static const int CELL_WIDTH = 80;  // Width of each cell
    static const int CELL_HEIGHT = 80; // Height of each cell

    std::vector<std::vector<Cell>> cells; // 2D array to store cells

    /**
     * @brief Constructs the grid by creating and positioning cells.
     * @param startX X-coordinate where the grid starts
     * @param startY Y-coordinate where the grid starts
     */
    Grid(int startX, int startY) {
        for (int i = 0; i < ROWS; ++i) {
            std::vector<Cell> row;
            for (int j = 0; j < COLUMNS; ++j) {
                row.emplace_back(startX + j * CELL_WIDTH, startY + i * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT);
            }
            cells.push_back(row);
        }
    }

    /**
     * @brief Draws the entire grid by drawing each individual cell.
     * @param renderer SDL_Renderer instance used to draw the grid.
     */
    void Draw(SDL_Renderer* renderer) {
        for (const auto& row : cells) {
            for (const auto& cell : row) {
                cell.Draw(renderer);
            }
        }
    }
};

#endif // GRID_HPP
