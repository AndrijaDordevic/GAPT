/*
#include <iostream>
#include <SDL3/SDL.h>

int gridCreate() {

    int grid[GRID_ROWS][GRID_COLUMNS] = { 0 }; // Initialize grid with all zeros

    // Calculate the total width and height for the grid
    int totalGridWidth = GRID_COLUMNS * CELL_WIDTH;
    int totalGridHeight = GRID_ROWS * CELL_HEIGHT;

    // Calculate the starting positions for centering the grid
    int startX = (WINDOW_WIDTH - totalGridWidth-300) / 2;
    int startY = (WINDOW_HEIGHT - totalGridHeight) / 2;

    while (running) {
        while (SDL_PollEvent(&event)) { // Process events in queue
            if (event.type == SDL_EVENT_QUIT) { // If user clicks 'X' or ALT+F4, quit the program
                running = false; // Stop the loop
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Sets background to white
        SDL_RenderClear(renderer); // Clears the screen

        // Draw the grid lines (black lines)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black color for grid lines

        // Draw vertical grid lines
        for (int i = 0; i <= GRID_COLUMNS; i++) {
            float x = startX + i * CELL_WIDTH;
            SDL_RenderLine(renderer, x, startY, x, startY + totalGridHeight); // Vertical lines
        }

        // Draw horizontal grid lines
        for (int i = 0; i <= GRID_ROWS; i++) {
            float y = startY + i * CELL_HEIGHT;
            SDL_RenderLine(renderer, startX, y, startX + totalGridWidth, y); // Horizontal lines
        }

        SDL_RenderPresent(renderer); // Render the new frame to the screen (double buffering)

        SDL_Delay(100); // Delay to control the frame rate (100ms)
    }
}
*/
