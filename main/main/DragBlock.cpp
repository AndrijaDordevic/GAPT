#include <SDL3/SDL.h>
#include "Client.hpp"     // For sending drag coordinates.
#include "Tetromino.hpp"  // Assuming this header defines the Tetromino type.
#include "Window.hpp"
#include <vector>
#include <algorithm>
#include "Audio.hpp"


int currentMaxLayer = 0;

void DragDrop(SDL_Event& event) {
    static Tetromino* draggedTetromino = nullptr;
    static int mouseOffsetX = 0, mouseOffsetY = 0;
    static SDL_Point originalPosition[4]; // Store original block positions

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Look for a tetromino that can be dragged.
        for (auto& tetromino : tetrominos) {
            if (!tetromino.canBeDragged)
                continue;
            for (size_t i = 0; i < tetromino.blocks.size(); ++i) {
                auto& block = tetromino.blocks[i];
                if (event.button.x >= block.x && event.button.x <= block.x + BLOCK_SIZE &&
                    event.button.y >= block.y && event.button.y <= block.y + BLOCK_SIZE) {

                    draggedTetromino = &tetromino;
                    // Save the original positions.
                    for (size_t j = 0; j < tetromino.blocks.size(); ++j) {
                        originalPosition[j] = { tetromino.blocks[j].x, tetromino.blocks[j].y };
                    }
                    mouseOffsetX = event.button.x - block.x;
                    mouseOffsetY = event.button.y - block.y;
                    draggingInProgress = true;
                    draggedTetromino->layer = ++currentMaxLayer;
					//Client::shape.erase(Client::shape.begin());
                    return;
                }
            }
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION && draggedTetromino) {
        // Calculate delta relative to the first block.
        int dx = event.motion.x - mouseOffsetX - draggedTetromino->blocks[0].x;
        int dy = event.motion.y - mouseOffsetY - draggedTetromino->blocks[0].y;
        for (auto& block : draggedTetromino->blocks) {
            block.x += dx;
            block.y += dy;
        }
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        if (draggedTetromino) {
            int gridStartX = 112;
            int gridStartY = 125;

            if (IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                // Snap to grid.
                for (auto& block : draggedTetromino->blocks) {
                    block.x = SnapToGrid(block.x, gridStartX);
                    block.y = SnapToGrid(block.y, gridStartY);
                }
            }

            // If there is a collision or the tetromino is outside the grid,
            // revert to the original position.
            if (CheckCollision(*draggedTetromino, placedTetrominos) ||
                !IsInsideGrid(*draggedTetromino, gridStartX, gridStartY)) {
                for (size_t i = 0; i < draggedTetromino->blocks.size(); ++i) {
                    draggedTetromino->blocks[i].x = originalPosition[i].x;
                    draggedTetromino->blocks[i].y = originalPosition[i].y;
                }
            }
            else {
                Client::sendDragCoordinates(*draggedTetromino);
                // Confirm the drop.
                draggedTetromino->canBeDragged = false;
                placedTetrominos.push_back(*draggedTetromino);
                auto it = std::remove_if(tetrominos.begin(), tetrominos.end(),
                    [&](const Tetromino& t) { return &t == draggedTetromino; });
                tetrominos.erase(it, tetrominos.end());
                Client::spawnedCount--;
                Audio::PlaySoundFile("Assets/Sounds/BlockPlace.mp3");

                // Send the updated coordinates to the server.
                
            }
            // Reset drag state.
            draggedTetromino = nullptr;
            draggingInProgress = false;
            ClearSpanningTetrominos(gridStartX, gridStartY, Columns, Columns);
        }
    }
}
