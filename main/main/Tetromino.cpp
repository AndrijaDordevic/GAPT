#include <SDL3/SDL.h>
#include "Window.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <memory>
#include "Tetromino.hpp"

// Global flags and spawn data
bool draggingInProgress = false;
bool CanDrag = false;
int spawnX = 0;
int spawnYPositions[3] = { 0, 0, 0 };
bool positionsOccupied[3] = { false, false, false };
int spawnedCount = 0;
int score = 0;
int POINTS_PER_BLOCK = 10;

// Store tetrominos by value instead of pointers.
// Make sure these are defined only once in your project.
std::vector<Tetromino> tetrominos;         // Active tetrominos (for drag, etc.)
std::vector<Tetromino> placedTetrominos;     // Tetrominos that have been placed
std::vector<Tetromino> lockedTetrominos;     // Tetrominos that are locked in place
std::vector<Block> placedIndividualBlocks;   // For individual block collision logic

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

    // Tetromino shapes (each shape is a vector of 4 SDL_Points)
    std::vector<std::vector<SDL_Point>> shapes = {
        {{0, 0}, {1, 0}, {2, 0}, {2, 0}},    //Horizontal Line
        {{0, 0}, {0, 1}, {1, 1}, {1, 1}},    // Smaller Bottom Left shape
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},    // Bottom Left shape
        {{0, 0}, {1, 0}, {1, 0}, {1, 1}},    // Smaller Right Top shape
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}},    // Right Top shape
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},    // Z shape
        {{0, 0}, {1, 0}, {0, 1}, {1, 1}},    // Square shape
        {{0, 0}, {1, 0}, {0, 1}, {0, 2}},    // Upside Down L 
        {{0, 0}, {1, 0}, {0, 1}, {0, 1}},    // Smaller Upside Down L shape
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}},    // Mirrored Upside Down L shape
        {{0, 0}, {1, 0}, {2, 0}, {0, 1}},    // Left Top  shape
        {{0, 0}, {1, 0}, {1, 0}, {0, 1}},    // Smaller Left Top  shape
        {{0, 0}, {0, 1}, {0, 2}, {0, 1}},    // 3 blocks i shape
        {{0, 0}, {0, 1}, {0, 1}, {0, 1}},    // 2 blocks i shape
        {{0, 0}, {0, 1}, {0, 2}, {1, 2}},    // L shape
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}},    // 4 shape
		{{0, 0}, {0, 0}, {0, 0}, {0, 0}},    // Dot shape

    };

    // Define an array of possible colors
    SDL_Color colors[] = {
        {255, 0, 0, 255},    // Red
        {0, 255, 0, 255},    // Green
        {0, 0, 255, 255},    // Blue
        {255, 255, 0, 255},  // Yellow
        {255, 165, 0, 255},   // Orange
		{100, 65,  0, 200},   // Brown
		{0, 255, 255, 255},  // Cyan
		{128, 0, 128, 255},  // Purple
		{255, 0, 255, 255},  // Magenta

    };

    int randomColorIndex = rand() % (sizeof(colors) / sizeof(colors[0]));
    int randomShapeIndex = rand() % shapes.size();

    Tetromino newTetromino;
    newTetromino.color = colors[randomColorIndex]; // Random color

    // Find a free spawn position among the three possible ones.
    int chosenSpawnY = -1;
    for (int i = 0; i < 3; ++i) {
        if (IsPositionFree(spawnYPositions[i])) {
            chosenSpawnY = spawnYPositions[i];
            break;
        }
    }
    if (chosenSpawnY == -1) {
        // All positions are occupied; do not spawn
        return;
    }

    // Mark the chosen spawn position as occupied
    int posIndex = (chosenSpawnY == spawnYPositions[0]) ? 0 :
        (chosenSpawnY == spawnYPositions[1]) ? 1 : 2;
    positionsOccupied[posIndex] = true;

    // Position the tetromino’s blocks based on the chosen shape and spawn coordinates
    for (const auto& point : shapes[randomShapeIndex]) {
        Block block = {
            spawnX + point.x * BLOCK_SIZE,
            chosenSpawnY + point.y * BLOCK_SIZE,
            newTetromino.color
        };
        newTetromino.blocks.push_back(block);
    }

    // Add the new tetromino to the active container.
    tetrominos.push_back(newTetromino);
    spawnedCount++;
}

void ReleaseOccupiedPositions() {
    // Reset positionsOccupied
    for (int i = 0; i < 3; ++i) {
        positionsOccupied[i] = false;
    }
    // Recalculate based on blocks in the active tetrominos
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

// Render function that now works with objects only.
void RenderTetrominos(SDL_Renderer* ren) {
    // Combine all tetrominos (active, placed, and locked) into one container for sorting.
    std::vector<std::reference_wrapper<const Tetromino>> allTetrominos;
    for (const auto& t : tetrominos) {
        allTetrominos.push_back(std::cref(t));  // Wrap with std::cref
    }
    for (const auto& t : placedTetrominos) {
        allTetrominos.push_back(std::cref(t));
    }
    for (const auto& t : lockedTetrominos) {
        allTetrominos.push_back(std::cref(t));
    }

    // Sort tetrominos by their layer value.
    std::sort(allTetrominos.begin(), allTetrominos.end(), [](const Tetromino& a, const Tetromino& b) {
        return a.layer < b.layer;
        });

    // Render each tetromino's blocks
    for (const auto& tetrominoRef : allTetrominos) {
        const Tetromino& tetromino = tetrominoRef.get();
        for (const auto& block : tetromino.blocks) {
            SDL_SetRenderDrawColor(ren, block.color.r, block.color.g, block.color.b, block.color.a);
            SDL_FRect rect = { static_cast<float>(block.x), static_cast<float>(block.y),
                               static_cast<float>(BLOCK_SIZE), static_cast<float>(BLOCK_SIZE) };
            SDL_RenderFillRect(ren, &rect);
        }
    }
}

// Snap value to nearest multiple of BLOCK_SIZE
int SnapToGrid(int value, int gridStart) {
    return gridStart + ((value - gridStart + BLOCK_SIZE / 2) / BLOCK_SIZE) * BLOCK_SIZE;
}

// Check if a tetromino collides with placed tetrominos or individual blocks.
bool CheckCollision(const Tetromino &tetro, const std::vector<Tetromino>& placedTetrominos) {
    // Check collision with each tetromino in the placedTetrominos vector.
    for (const auto &placed : placedTetrominos) {
        for (const auto &placedBlock : placed.blocks) {
            for (const auto &block : tetro.blocks) {
                if (block.x == placedBlock.x && block.y == placedBlock.y)
                    return true;  // Collision detected
            }
        }
    }
    return false; // No collision detected
}

// Check if tetromino is inside grid boundaries.
bool IsInsideGrid(const Tetromino& tetro, int gridStartX, int gridStartY) {
    for (const auto& block : tetro.blocks) {
        if (block.x < gridStartX - 5 || block.y < gridStartY - 5 ||
            block.x > gridStartX + (Columns * BLOCK_SIZE) - 20 ||
            block.y > gridStartY + (Columns * BLOCK_SIZE) - 20)
        {
            return false;
        }
    }
    return true;
}

// Add blocks from tetromino to the individual blocks container.
void AddToIndividualBlocks(const Tetromino& tetro) {
    for (const auto& block : tetro.blocks) {
        placedIndividualBlocks.push_back(block);
    }
}

// This function is called in your main loop to handle tetromino spawning.
void RunBlocks(SDL_Renderer* renderer) {
    static bool initialized = false;

    // Set spawn positions
    spawnX = WINDOW_WIDTH - BLOCK_SIZE - 250;
    spawnYPositions[0] = WINDOW_HEIGHT/3 - 250;
    spawnYPositions[1] = 2 * (WINDOW_HEIGHT / 3) - 300;
    spawnYPositions[2] = WINDOW_HEIGHT - 300;

    // Spawn a new tetromino if conditions are met.
    if (!initialized) {
        // Initial spawning of 3 tetrominos
        for (int i = 0; i < 3; i++) {
            SpawnTetromino();
        }
        initialized = true;
    }

    // Check for open spawn positions and refill up to 3 active tetrominos
    while (tetrominos.size() < 3) {
        SpawnTetromino();
    }
}

void ClearSpanningTetrominos(int gridStartX, int gridStartY, int gridCols, int gridRows) {
    // Create an occupancy grid for rows and columns.
    std::vector<std::vector<bool>> occupancy(gridRows, std::vector<bool>(gridCols, false));

    // Lambda to mark occupancy for a given tetromino container.
    auto markOccupancy = [&](const std::vector<Tetromino>& tetrominoContainer) {
        for (const auto& tetro : tetrominoContainer) {
            for (const auto& block : tetro.blocks) {
                int col = (block.x - gridStartX) / BLOCK_SIZE;
                int row = (block.y - gridStartY) / BLOCK_SIZE;
                if (col >= 0 && col < gridCols && row >= 0 && row < gridRows)
                    occupancy[row][col] = true;
            }
        }
        };

    // Mark occupancy from both placed and locked tetrominos.
    markOccupancy(placedTetrominos);
    markOccupancy(lockedTetrominos);

    // Determine which rows and columns are completely filled.
    std::vector<bool> completeRows(gridRows, true);
    std::vector<bool> completeCols(gridCols, true);

    // Check each row.
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridCols; c++) {
            if (!occupancy[r][c]) {
                completeRows[r] = false;
                break;
            }
        }
    }

    // Check each column.
    for (int c = 0; c < gridCols; c++) {
        for (int r = 0; r < gridRows; r++) {
            if (!occupancy[r][c]) {
                completeCols[c] = false;
                break;
            }
        }
    }

    // Count how many rows and columns are completely filled.
    int numRowsCleared = 0, numColsCleared = 0;
    for (bool rowComplete : completeRows) {
        if (rowComplete) numRowsCleared++;
    }
    for (bool colComplete : completeCols) {
        if (colComplete) numColsCleared++;
    }
    int totalClearedLines = numRowsCleared + numColsCleared;

    // Calculate bonus multiplier:
    // For example, 1 line cleared gives 1.0x, 2 lines give 1.5x, 3 lines give 2.0x, etc.
    double multiplier = 1.0;
    if (totalClearedLines > 1) {
        multiplier = 1.0 + 0.5 * (totalClearedLines - 1);
    }

    // Remove blocks that are in any complete row or column and accumulate the removed block count.
    int removedTotal = 0;
    auto removeClearedBlocks = [&](std::vector<Tetromino>& container) {
        for (auto& tetro : container) {
            auto it = std::remove_if(tetro.blocks.begin(), tetro.blocks.end(),
                [&](const Block& block) {
                    int col = (block.x - gridStartX) / BLOCK_SIZE;
                    int row = (block.y - gridStartY) / BLOCK_SIZE;
                    bool removeRow = (row >= 0 && row < gridRows && completeRows[row]);
                    bool removeCol = (col >= 0 && col < gridCols && completeCols[col]);
                    return removeRow || removeCol;
                });
            int removedCount = std::distance(it, tetro.blocks.end());
            removedTotal += removedCount;
            tetro.blocks.erase(it, tetro.blocks.end());
        }
        // Remove any tetromino that has lost all of its blocks.
        container.erase(std::remove_if(container.begin(), container.end(),
            [](const Tetromino& t) { return t.blocks.empty(); }),
            container.end());
        };

    // Remove cleared blocks from both containers.
    removeClearedBlocks(placedTetrominos);
    removeClearedBlocks(lockedTetrominos);

    // Update score: Apply the bonus multiplier if multiple lines were cleared.
    int baseScore = removedTotal * POINTS_PER_BLOCK;
    int scoreGain = static_cast<int>(baseScore * multiplier);
    score += scoreGain;
	cout << "Score: " << score << endl;
}