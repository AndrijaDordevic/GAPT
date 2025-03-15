#include <iostream>
#include <SDL3/SDL.h>
#include "Window.hpp"
#include "Blocks.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>

const int BLOCK_SIZE = 80;
const int Columns = 10;
bool draggingInProgress = false; // Prevents spawning while dragging
bool CanDrag = false;

struct Block {
    int x, y;
    SDL_Color color;
    bool dragging = false;
};

struct Tetromino {
    std::vector<Block> blocks;
    SDL_Color color;
    bool canBeDragged = true; // Flag to control drag state
};

std::vector<Tetromino> tetrominos;
std::vector<Tetromino*> placedTetrominos;
// Track locked Tetrominos that can't be moved
std::vector<Tetromino*> lockedTetrominos;

// Define 3 fixed spawn positions on the right side
int spawnX;  // Moved further to the left
int spawnYPositions[3]; // Further left adjustment

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
    if (draggingInProgress) return;

    // Tetromino shapes (4 blocks in each configuration)
    std::vector<std::vector<SDL_Point>> shapes = {
        {{0, 0}, {1, 0}, {2, 0}, {3, 0}},    // I shape
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},    // O shape
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}},    // L shape
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},    // Z shape
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}}     // T shape
    };

    // Define an array of possible colors
    SDL_Color colors[] = {
        {255, 0, 0, 255},    // Red
        {0, 255, 0, 255},    // Green
        {0, 0, 255, 255},    // Blue
        {255, 255, 0, 255},  // Yellow
        {255, 165, 0, 255}   // Orange
    };

    // Select a random index for the color
    int randomColorIndex = rand() % (sizeof(colors) / sizeof(colors[0]));

    // Randomly choose a tetromino shape
    int randomIndex = rand() % shapes.size();
    Tetromino newTetromino;
    newTetromino.color = colors[randomColorIndex]; // Set the color randomly

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

void RenderTetrominos(SDL_Renderer* ren) {
    for (const auto& tetromino : tetrominos) {
        for (const auto& block : tetromino.blocks) {
            SDL_SetRenderDrawColor(ren, block.color.r, block.color.g, block.color.b, block.color.a);
            SDL_FRect rect = { static_cast<float>(block.x), static_cast<float>(block.y), static_cast<float>(BLOCK_SIZE), static_cast<float>(BLOCK_SIZE) };
            SDL_RenderFillRect(ren, &rect);
        }
    }
}
// Function to snap a value to the nearest multiple of BLOCK_SIZE
int SnapToGrid(int value, int gridStart) {
    return gridStart + ((value - gridStart + BLOCK_SIZE / 2) / BLOCK_SIZE) * BLOCK_SIZE;
}

// Function to check if a Tetromino overlaps any placed Tetromino

bool CheckCollision(const Tetromino& tetro, const std::vector<Tetromino*>& placedTetrominos) {
    for (const auto& placed : placedTetrominos) {
        for (const auto& placedBlock : placed->blocks) {
            for (const auto& block : tetro.blocks) {
                if (block.x == placedBlock.x && block.y == placedBlock.y) {
                    return true; // Precise overlap detection
                }
            }
        }
    }
    return false;
}



// Helper function to check if Tetromino is fully inside the grid
bool IsInsideGrid(const Tetromino& tetro, int gridStartX, int gridStartY) {
    for (const auto& block : tetro.blocks) {
        if (block.x < gridStartX || block.y < gridStartY || block.x >= gridStartX + Columns * BLOCK_SIZE || block.y >= gridStartY + Columns * BLOCK_SIZE) {
            return false;
        }
    }
    return true;
}

// Updated DragDrop function
void DragDrop(SDL_Event& event) {
    static Tetromino* draggedTetromino = nullptr;
    static int mouseOffsetX = 0, mouseOffsetY = 0;
    static SDL_Point originalPosition[4]; // Store original block positions

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Check if any draggable tetromino was clicked
        for (auto& tetromino : tetrominos) {
            if (!tetromino.canBeDragged) { // Skip if not draggable
                continue;
            }

            for (size_t i = 0; i < tetromino.blocks.size(); ++i) {
                auto& block = tetromino.blocks[i];
                if (event.button.x >= block.x && event.button.x <= block.x + BLOCK_SIZE &&
                    event.button.y >= block.y && event.button.y <= block.y + BLOCK_SIZE) {

                    draggedTetromino = &tetromino;
                    for (size_t j = 0; j < tetromino.blocks.size(); ++j) {
                        originalPosition[j] = { tetromino.blocks[j].x, tetromino.blocks[j].y };
                    }
                    mouseOffsetX = event.button.x - block.x;
                    mouseOffsetY = event.button.y - block.y;
                    draggingInProgress = true;
                    return;
                }
            }
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION && draggedTetromino) {
        int dx = event.motion.x - mouseOffsetX - draggedTetromino->blocks[0].x;
        int dy = event.motion.y - mouseOffsetY - draggedTetromino->blocks[0].y;

        for (auto& block : draggedTetromino->blocks) {
            block.x += dx;
            block.y += dy;
        }
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        if (draggedTetromino) {
            // Place the Tetromino and disable dragging once placed
            int gridStartX = (WINDOW_WIDTH - (Columns * CELL_WIDTH)) / 2 - 100;
            int gridStartY = OFFSET;

            for (auto& block : draggedTetromino->blocks) {
                block.x = SnapToGrid(block.x, gridStartX);
                block.y = SnapToGrid(block.y, gridStartY);
            }

            if (CheckCollision(*draggedTetromino, placedTetrominos) || !IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                // Reset position if collision or outside grid
                for (size_t i = 0; i < draggedTetromino->blocks.size(); ++i) {
                    draggedTetromino->blocks[i].x = originalPosition[i].x;
                    draggedTetromino->blocks[i].y = originalPosition[i].y;
                }
            }
            else {
                // Successfully placed, set canBeDragged to false
                draggedTetromino->canBeDragged = false; // Disable dragging after placement
                placedTetrominos.push_back(draggedTetromino);
                spawnedCount--; // Decrement spawned count on successful placement
            }

            draggedTetromino = nullptr;
            draggingInProgress = false;
        }
    }
}






void RunBlocks(SDL_Renderer* renderer) {
    static Uint32 lastSpawnTime = SDL_GetTicks();

    // Set spawn positions (ensure these are initialized once if possible)
    spawnX = WINDOW_WIDTH - BLOCK_SIZE - 250;
    spawnYPositions[0] = 180;
    spawnYPositions[1] = 180 + BLOCK_SIZE + 200;
    spawnYPositions[2] = 180 + 2 * (BLOCK_SIZE + 180);

    // Spawn new tetromino if conditions are met
    if (SDL_GetTicks() - lastSpawnTime > 1000 && spawnedCount < 3) {
        SpawnTetromino();
        
    } 
    if(spawnedCount == 3){
        CanDrag = true;
    }
}
