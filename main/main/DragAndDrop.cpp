#include <SDL3/SDL.h>
#include <iostream>

using namespace std;

//Variables
int Width = 800;
int Height = 500;


//Creating shape
SDL_FRect createShape()
{
    SDL_FRect rect;
    rect.x = 10.0f;
    rect.y = 10.0f;
    rect.w = 20.0f;
    rect.h = 20.0f;
    return rect;
}



int test() {

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    // Create window and renderer
    SDL_Window* window;
    SDL_Renderer* renderer;

    if (SDL_CreateWindowAndRenderer("title", Width, Height, SDL_WINDOW_RESIZABLE, &window, &renderer) < 0) {
        cout << "Window/Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
        SDL_Quit();
        return 1;
    }

    SDL_FRect square = createShape();

    bool running = true;
    SDL_Event event;

    // To track if the square is being dragged
    bool dragging = false;

    // Where the mouse clicks on the square
    float MousePosX = 0, MousePosY = 0;

    // Main loop
    while (running) {
        while (SDL_PollEvent(&event)) {  //Loops until it quits
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            // Mouse button down event (start dragging)
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {

                    // Check if the mouse click is within the square's bounds
                    if (event.button.x >= square.x && event.button.x <= square.x + square.w &&
                        event.button.y >= square.y && event.button.y <= square.y + square.h) {
                        dragging = true;

                        // Calculate the offset where the mouse clicked relative to the top-left corner of the square
                        MousePosX = event.button.x - square.x;
                        MousePosY = event.button.y - square.y;
                    }
                }
            }

            // Mouse motion event (dragging)
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (dragging) {
                    // Update the position of the square to follow the mouse
                    square.x = event.motion.x - MousePosX;
                    square.y = event.motion.y - MousePosY;
                }
            }

            // Mouse button up event (stop dragging)
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    dragging = false;
                }
            }
        }

        // Clear the screen with black color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw the square with red color
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red color for the square
        SDL_RenderFillRect(renderer, &square);  // Pass the address of the square (SDL_FRect)

        // Present the updated renderer to the screen
        SDL_RenderPresent(renderer);

        SDL_Delay(16);  // Control the frame rate (about 60 FPS)
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}