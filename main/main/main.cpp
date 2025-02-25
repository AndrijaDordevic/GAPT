#include "Window.hpp" // Include the custom header file that contains SDL initialization and cleanup functions

int main() {
    SDL_Window* window = nullptr;    // Pointer to the SDL window
    SDL_Renderer* renderer = nullptr; // Pointer to the SDL renderer

    // Initialize SDL and create a window and renderer
    // The function 'initializeSDL' returns false if initialization fails
    if (!initializeSDL(window, renderer)) {
        return -1; // Exit the program with an error code if initialization fails
    }

    bool running = true; // Boolean flag to keep track of whether the application is running

    // Main loop that runs until the user decides to exit
    while (running) {
        // Process user input events (such as quitting the application)
        handleEvents(running);

        // Set the renderer's drawing color to black (RGBA: 0, 0, 0, 255)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear the window and fill it with the set drawing color (black in this case)
        SDL_RenderClear(renderer);

        // Present the rendered frame to the screen (double buffering)
        SDL_RenderPresent(renderer);

        // Introduce a small delay (100 milliseconds) to control the frame rate
        SDL_Delay(100);
    }

    // Clean up and free allocated resources before exiting
    cleanupSDL(window, renderer);

    return 0; // Return 0 to indicate successful program execution
}
