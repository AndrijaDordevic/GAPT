#include <SDL3/SDL.h>
#include <vector>
#include <cstdlib>
#include <ctime>


using namespace std;


const int SCREEN_WIDTH = 1000;   // Fixed width
const int SCREEN_HEIGHT = 600;   // Fixed height
const int BLOCK_SIZE = 40;       // Block size

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;


struct Block {
    int x, y;
    SDL_Color color;
    bool dragging = false;
};

struct Tetromino {
    std::vector<Block> blocks;
    SDL_Color color;
};

std::vector<Tetromino> tetrominos;

// Define 3 fixed spawn positions on the right side
const int spawnX = SCREEN_WIDTH - BLOCK_SIZE - 130;  // Moved further to the left
const int spawnYPositions[] = { 90, SCREEN_HEIGHT / 2 - BLOCK_SIZE / 2, SCREEN_HEIGHT - BLOCK_SIZE - 100 }; // Further left adjustment

bool positionsOccupied[] = { false, false, false }; // Track if top, middle, and bottom positions are occupied

int spawnedCount = 0; // Track how many tetrominoes have been spawned

// Function to check if a spawn position is free
bool IsPositionFree(int spawnY) {
    for (const auto& tetromino : tetrominos) {
        for (const auto& block : tetromino.blocks) {
            if (block.x == spawnX && block.y == spawnY) {
                return false; // Position is already occupied
            }
        }
    }
    return true; // Position is free
}

void SpawnTetromino() {
    if (spawnedCount >= 3) return; // Stop spawning after 3 Tetrominoes

    // Tetromino shapes (4 blocks in each configuration)
    std::vector<std::vector<SDL_Point>> shapes = {
        {{0, 0}, {1, 0}, {2, 0}, {3, 0}},    // I shape
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},    // O shape
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}},    // L shape
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},    // Z shape
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}}     // T shape
    };

    SDL_Color colors[] = {
        {255, 0, 0, 255},    // Red
        {0, 255, 0, 255},    // Green
        {0, 0, 255, 255},    // Blue
        {255, 255, 0, 255},  // Yellow
        {255, 165, 0, 255}   // Orange
    };

    int randomIndex = rand() % shapes.size(); // Select random Tetromino shape
    Tetromino newTetromino;
    newTetromino.color = colors[randomIndex];

    // Find the first free position starting from the top
    int spawnY = -1;
    for (int i = 0; i < 3; ++i) {
        if (IsPositionFree(spawnYPositions[i])) {
            spawnY = spawnYPositions[i];
            break; // Exit once a free spot is found
        }
    }

    if (spawnY == -1) {
        // If all positions are occupied, return without spawning
        return;
    }

    // Mark the chosen spawn position as occupied
    positionsOccupied[spawnY == spawnYPositions[0] ? 0 : spawnY == spawnYPositions[1] ? 1 : 2] = true;

    // Position the tetromino at the fixed X (right side) and the chosen Y (based on spawn logic)
    for (const auto& point : shapes[randomIndex]) {
        Block block = { spawnX + point.x * BLOCK_SIZE, spawnY + point.y * BLOCK_SIZE, newTetromino.color };
        newTetromino.blocks.push_back(block);
    }

    tetrominos.push_back(newTetromino); // Add the new Tetromino to the list
    spawnedCount++; // Increment spawned counter
}

// Function to release positions once Tetrominoes move down
void ReleaseOccupiedPositions() {
    for (int i = 0; i < 3; ++i) {
        positionsOccupied[i] = false;
    }

    // Mark positions as occupied only if the corresponding Y positions are filled
    for (const auto& tetromino : tetrominos) {
        for (const auto& block : tetromino.blocks) {
            if (block.x == spawnX) {
                if (block.y == spawnYPositions[0]) positionsOccupied[0] = true;
                if (block.y == spawnYPositions[1]) positionsOccupied[1] = true;
                if (block.y == spawnYPositions[2]) positionsOccupied[2] = true;
            }
        }
    }
}

void RenderTetrominos() {
    for (const auto& tetromino : tetrominos) {
        for (const auto& block : tetromino.blocks) {
            SDL_SetRenderDrawColor(renderer, block.color.r, block.color.g, block.color.b, block.color.a);
            SDL_FRect rect = { static_cast<float>(block.x), static_cast<float>(block.y), static_cast<float>(BLOCK_SIZE), static_cast<float>(BLOCK_SIZE) };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void DragDrop(SDL_Event& event) {

    // Static variables to track the dragged tetromino and mouse offsets
    static Tetromino* draggedTetromino = nullptr; // Pointer to the currently dragged tetromino
    static int mouseOffsetX = 0, mouseOffsetY = 0; // Offset between mouse click and top-left corner of the tetromino
    static int initialX, initialY; // Store the original position of the first block (top-left corner)

    // Check if the left mouse button is pressed
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Iterate through all tetrominos to find the one being clicked
        for (auto& tetromino : tetrominos) {
            // Iterate through all blocks in the current tetromino
            for (auto& block : tetromino.blocks) {
                // Check if the mouse click is within the bounds of the current block
                if (event.button.x >= block.x && event.button.x <= block.x + BLOCK_SIZE &&
                    event.button.y >= block.y && event.button.y <= block.y + BLOCK_SIZE) {

                    // Set the draggedTetromino to the current tetromino
                    draggedTetromino = &tetromino;

                    // Find the top-left corner of the tetromino for accurate movement
                    int minX = tetromino.blocks[0].x; // Initialize with the first block's position
                    int minY = tetromino.blocks[0].y;
                    for (auto& b : tetromino.blocks) {
                        if (b.x < minX) minX = b.x; // Update minX if a block with a smaller x is found
                        if (b.y < minY) minY = b.y; // Update minY if a block with a smaller y is found
                    }

                    // Calculate the offset between the mouse click and the top-left corner of the tetromino
                    mouseOffsetX = event.button.x - minX;
                    mouseOffsetY = event.button.y - minY;

                    // Store the initial position of the top-left corner for later calculations
                    initialX = minX;
                    initialY = minY;

                    return; // Exit the function after finding the clicked block
                }
            }
        }
    }

    // Check if the mouse is moving and a tetromino is being dragged
    if (event.type == SDL_EVENT_MOUSE_MOTION && draggedTetromino) {
        // Calculate the new position of the tetromino based on the mouse movement
        int newX = event.motion.x - mouseOffsetX;
        int newY = event.motion.y - mouseOffsetY;

        // Calculate the change in position (delta) from the initial position
        int dx = newX - initialX;
        int dy = newY - initialY;

        // Move all blocks in the dragged tetromino by the delta
        for (auto& block : draggedTetromino->blocks) {
            block.x += dx;
            block.y += dy;
        }

        // Update the reference point for the next movement
        initialX = newX;
        initialY = newY;
    }

    // Check if the left mouse button is released
    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        // Reset the draggedTetromino pointer to stop dragging
        draggedTetromino = nullptr;
    }
}




int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(0)));
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Tetris Block Spawner", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, nullptr);

    bool running = true;
    SDL_Event event;
    Uint32 lastSpawnTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            DragDrop(event); // <--- Call the function here
        }

        if (SDL_GetTicks() - lastSpawnTime > 1000 && spawnedCount < 3) {
            SpawnTetromino();
            lastSpawnTime = SDL_GetTicks();
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        RenderTetrominos();

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
