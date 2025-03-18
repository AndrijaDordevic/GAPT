#include <SDL3/SDL.h>
#include "Window.hpp"
#include <vector>
#include "Tetromino.hpp"

void DragDrop(SDL_Event& event) {
    static Tetromino* draggedTetromino = nullptr;
    static int mouseOffsetX = 0, mouseOffsetY = 0;
    static SDL_Point originalPosition[4]; // Store original block positions

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        for (auto& tetromino : tetrominos) {
            if (!tetromino.canBeDragged) continue; // Skip if not draggable

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

                    // **Increase layer so it's drawn on top**
                    draggedTetromino->layer = 1;

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
            int gridStartX = (WINDOW_WIDTH - (GRID_COLUMNS * CELL_WIDTH)) / 2 - 100;
            int gridStartY = OFFSET;

            if (IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                for (auto& block : draggedTetromino->blocks) {
                    block.x = SnapToGrid(block.x, gridStartX);
                    block.y = SnapToGrid(block.y, gridStartY);
                }
            }

            if (CheckCollision(*draggedTetromino, placedTetrominos) ||
                !IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                // Reset position if collision or outside grid
                for (size_t i = 0; i < draggedTetromino->blocks.size(); ++i) {
                    draggedTetromino->blocks[i].x = originalPosition[i].x;
                    draggedTetromino->blocks[i].y = originalPosition[i].y;
                }
            }
            else {
                // Successfully placed, disable dragging
                draggedTetromino->canBeDragged = false;
                placedTetrominos.push_back(draggedTetromino);
                spawnedCount--;

                // **Reset layer after placement**
                draggedTetromino->layer = 0;

                // **Track individual blocks for collision handling**
                AddToIndividualBlocks(*draggedTetromino);
            }

            draggedTetromino = nullptr;
            draggingInProgress = false;
        }
    }
}
