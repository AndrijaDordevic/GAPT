#include <iostream> // For console Output
#include <SDL3/SDL.h> // Include the SDL3 Library
using namespace std; // Removes redundance of std


const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 900;
const int GRID_ROWS = 10;
const int GRID_COLUMNS = 10;
const int OFFSET = 100;
const int CELL_WIDTH = 80;
const int CELL_HEIGHT = 80;

int grid[GRID_ROWS][GRID_COLUMNS] = { 0 }; // Initialize grid with all zeros

int main() {
    // Creation of SDL Window with size 1400x900 and is resizable
    SDL_Window* window = SDL_CreateWindow("Block Game", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

    // Check if window creation failed
    if (!window) {
        cerr << "Failed to create window: " << SDL_GetError() << endl;
        return -1; // Exit the program with an error code
    }

    // Create a renderer to handle drawing graphics
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    // Check if renderer creation failed
    if (!renderer) {
        cerr << "Failed to create renderer: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window); // Destroy the window before exiting
        return -1; // Exit with an error code
    }

    // Calculate the total width and height for the grid
    int totalGridWidth = GRID_COLUMNS * CELL_WIDTH;
    int totalGridHeight = GRID_ROWS * CELL_HEIGHT;

    // Calculate the starting positions for centering the grid
    int startX = (WINDOW_WIDTH - totalGridWidth-300) / 2;
    int startY = (WINDOW_HEIGHT - totalGridHeight) / 2;

    // Main event loop flag
    bool running = true;
    SDL_Event event; // SDL event structure for handling inputs

    // Main loop to keep the window running until closed
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

    // Cleanup resources before exiting
    SDL_DestroyRenderer(renderer); // Destroys the renderer (frees GPU resources)
    SDL_DestroyWindow(window); // Destroys the window (frees memory)
    SDL_Quit(); // Shutdowns SDL completely

    return 0; // Exit the program successfully
}
