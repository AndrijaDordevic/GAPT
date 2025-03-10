    #include "Window.hpp"  // Includes drawGrid() declaration
    #include "Menu.hpp"


int main() {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!initializeSDL(window, renderer)) {
        cerr << "Failed to initialize SDL." << endl;
        return 1;
    }
    srand(static_cast<unsigned>(time(0)));
        
    // Call DrawGrid(), which now runs the entire game loop
    DrawGrid(renderer);

    cleanupSDL(window, renderer);
    return 0;
}

