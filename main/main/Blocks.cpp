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
};

std::vector<Tetromino> tetrominos;
std::vector<Tetromino*> placedTetrominos;

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

void SpawnTetrominos() {
    if (!tetrominos.empty()) return; // Do not spawn if there are still active tetrominoes

    // Tetromino shapes
    std::vector<std::vector<SDL_Point>> shapes = {
        {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, // I shape
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}}, // O shape
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}}, // L shape
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}}, // Z shape
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}}  // T shape
    };

    // Define colours
    SDL_Color colors[] = {
        {255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 0, 255}, {255, 165, 0, 255}
    };

    for (int i = 0; i < 3; ++i) {
        Tetromino newTetromino;
        int randomShape = rand() % shapes.size();
        int randomColor = rand() % (sizeof(colors) / sizeof(colors[0]));

        newTetromino.color = colors[randomColor];

        // Assign blocks to the tetromino
        for (const auto& point : shapes[randomShape]) {
            Block block = { spawnX + point.x * BLOCK_SIZE, spawnYPositions[i] + point.y * BLOCK_SIZE, newTetromino.color };
            newTetromino.blocks.push_back(block);
        }

        tetrominos.push_back(newTetromino);
    }

    spawnedCount = 3; // Ensure three tetrominoes are tracked
    CanDrag = true; // Allow dragging after spawning
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

// Track locked Tetrominos that can't be moved
std::vector<Tetromino*> lockedTetrominos;

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
            // Skip locked Tetrominos
            if (std::find(lockedTetrominos.begin(), lockedTetrominos.end(), &tetromino) != lockedTetrominos.end()) {
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
            int gridStartX = (WINDOW_WIDTH - (Columns * CELL_WIDTH)) / 2 - 100;
            int gridStartY = OFFSET;

            for (auto& block : draggedTetromino->blocks) {
                block.x = SnapToGrid(block.x, gridStartX);
                block.y = SnapToGrid(block.y, gridStartY);
            }

            if (CheckCollision(*draggedTetromino, placedTetrominos) || !IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                // Reset position if collision or out of bounds
                for (size_t i = 0; i < draggedTetromino->blocks.size(); ++i) {
                    draggedTetromino->blocks[i].x = originalPosition[i].x;
                    draggedTetromino->blocks[i].y = originalPosition[i].y;
                }
            }
            else {
                if (std::find(lockedTetrominos.begin(), lockedTetrominos.end(), draggedTetromino) == lockedTetrominos.end()) {
                    lockedTetrominos.push_back(draggedTetromino);
                }
                placedTetrominos.push_back(draggedTetromino);
                spawnedCount--;

                if (spawnedCount == 0) { // If all are placed, reset
                    tetrominos.clear();
                    placedTetrominos.clear();
                    lockedTetrominos.clear();
                }
            }

            draggedTetromino = nullptr;
            draggingInProgress = false;
        }
    }

}






void RunBlocks(SDL_Renderer* renderer) {
    spawnX = WINDOW_WIDTH - BLOCK_SIZE - 250;
    spawnYPositions[0] = 180;
    spawnYPositions[1] = 180 + BLOCK_SIZE + 200;
    spawnYPositions[2] = 180 + 2 * (BLOCK_SIZE + 180);

    if (spawnedCount == 0) { // All tetrominoes placed
        SpawnTetrominos();
    }
}


