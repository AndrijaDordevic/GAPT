#ifndef WINDOW_H
#define WINDOW_H

// ==========================
//        INCLUDES
// ==========================
#include <iostream>   // Standard input/output library for debugging and error messages
#include <SDL3/SDL.h> // SDL3 library for handling graphics, window creation, and rendering

using namespace std; // Avoids the need to use "std::" before standard functions (e.g., cout, cerr)

// ==========================
//    WINDOW CONFIGURATION
// ==========================

// Constants defining the window size
const int WINDOW_WIDTH = 1600;  // The width of the window in pixels
const int WINDOW_HEIGHT = 950;  // The height of the window in pixels

// Constants defining the grid layout inside the window
const int GRID_ROWS = 10;       // Number of rows in the grid
const int GRID_COLUMNS = 10;    // Number of columns in the grid
const int OFFSET = 100;         // Offset (padding) around the grid for positioning
const int CELL_WIDTH = 80;      // The width of each grid cell
const int CELL_HEIGHT = 80;     // The height of each grid cell

// ==========================
//     FUNCTION DECLARATIONS
// ==========================

/**
 * @brief Initializes SDL, creates a window, and sets up a renderer.
 *
 * @param window   Reference to the SDL_Window pointer to be initialized
 * @param renderer Reference to the SDL_Renderer pointer to be initialized
 * @return         `true` if SDL initializes successfully, `false` otherwise
 */
bool initializeSDL(SDL_Window*& window, SDL_Renderer*& renderer);

/**
 * @brief Handles user input events, such as quitting the application.
 *
 * @param running Reference to the boolean flag that determines if the main loop should continue running
 */
void handleEvents(bool& running);

/**
 * @brief Cleans up SDL resources before exiting the application.
 *
 * @param window   Pointer to the SDL_Window to be destroyed
 * @param renderer Pointer to the SDL_Renderer to be destroyed
 */
void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer);

// Function to draw the game grid
void DrawGrid(SDL_Renderer* renderer);

#endif // WINDOW_H
