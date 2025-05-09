#ifndef WINDOW_H
#define WINDOW_H


#include <iostream>   
#include <SDL3/SDL.h> 

using namespace std;



// Constants defining the window size
const int WINDOW_WIDTH = 1600;  // The width of the window in pixels
const int WINDOW_HEIGHT = 950;  // The height of the window in pixels

// Constants defining the grid layout inside the window
const int GRID_ROWS = 9;       // Number of rows in the grid
const int GRID_COLUMNS = 9;    // Number of columns in the grid
const int OFFSET = 100;         // Offset (padding) around the grid for positioning
const int CELL_WIDTH = 81;      // The width of each grid cell
const int CELL_HEIGHT = 81;     // The height of each grid cell


bool initializeSDL(SDL_Window*& window, SDL_Renderer*& renderer);


void handleEvents(bool& running);


void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer);



#endif
