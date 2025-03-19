#include <SDL3/SDL.h>
#include "Window.hpp"
#include <vector>
#include "Tetromino.hpp"
#include <algorithm>

// Define the global layer counter (place this in one source file, e.g., Tetromino.cpp)
int currentMaxLayer = 0;

void DragDrop(SDL_Event& event) {
    static Tetromino* draggedTetromino = nullptr;
    static int mouseOffsetX = 0, mouseOffsetY = 0;
    static SDL_Point originalPosition[4]; // Store original block positions

    // Start drag: on mouse button down
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        for (auto& tetromino : tetrominos) {
            if (!tetromino.canBeDragged) continue; // Skip if not draggable

            for (size_t i = 0; i < tetromino.blocks.size(); ++i) {
                auto& block = tetromino.blocks[i];
                if (event.button.x >= block.x && event.button.x <= block.x + BLOCK_SIZE &&
                    event.button.y >= block.y && event.button.y <= block.y + BLOCK_SIZE) {

                    draggedTetromino = &tetromino;
                    // Save original positions for potential reset
                    for (size_t j = 0; j < tetromino.blocks.size(); ++j) {
                        originalPosition[j] = { tetromino.blocks[j].x, tetromino.blocks[j].y };
                    }
                    mouseOffsetX = event.button.x - block.x;
                    mouseOffsetY = event.button.y - block.y;
                    draggingInProgress = true;

                    // Assign a unique layer so it's drawn on top during drag
                    draggedTetromino->layer = ++currentMaxLayer;
                    return;
                }
            }
        }
    }

    // Handle dragging motion: on mouse motion event
    if (event.type == SDL_EVENT_MOUSE_MOTION && draggedTetromino) {
        int dx = event.motion.x - mouseOffsetX - draggedTetromino->blocks[0].x;
        int dy = event.motion.y - mouseOffsetY - draggedTetromino->blocks[0].y;
        for (auto& block : draggedTetromino->blocks) {
            block.x += dx;
            block.y += dy;
        }
    }

    // End drag: on mouse button up
    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        if (draggedTetromino) {
            int gridStartX = (WINDOW_WIDTH - (GRID_COLUMNS * CELL_WIDTH)) / 2 - 100;
            int gridStartY = OFFSET;

            // Snap blocks to grid if inside the grid
            if (IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                for (auto& block : draggedTetromino->blocks) {
                    block.x = SnapToGrid(block.x, gridStartX);
                    block.y = SnapToGrid(block.y, gridStartY);
                }
            }

            // Check for collisions or out-of-bound placement
            if (CheckCollision(*draggedTetromino, placedTetrominos) ||
                !IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                // Reset position if collision or outside grid
                for (size_t i = 0; i < draggedTetromino->blocks.size(); ++i) {
                    draggedTetromino->blocks[i].x = originalPosition[i].x;
                    draggedTetromino->blocks[i].y = originalPosition[i].y;
                }
            }
            else {
                // Successfully placed tetromino:
                // 1. Disable further dragging.
                draggedTetromino->canBeDragged = false;
                // 2. Push a copy of the dragged tetromino to the placed tetrominos vector.
                placedTetrominos.push_back(*draggedTetromino);
                // 3. Remove the tetromino from the active tetrominos vector.
                auto it = std::remove_if(tetrominos.begin(), tetrominos.end(),
                    [&](const Tetromino& t) { return &t == draggedTetromino; });
                tetrominos.erase(it, tetrominos.end());
                // 4. Add individual blocks for collision handling if needed.
                // Decrement the spawned count since it has been placed.
                spawnedCount--;
                // Do not reset the layer here; leave it as the high value assigned.
            }

            // End dragging
            draggedTetromino = nullptr;
            draggingInProgress = false;

            ClearSpanningTetrominos(gridStartX, gridStartY, Columns, Columns);
        }
    }
}
