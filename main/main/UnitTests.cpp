#include <cassert>
#include "Tetromino.hpp"
#include <vector>
#include <iostream>

std::vector<std::vector<SDL_Point>> shapes = {
{{0, 0}, {1, 0}, {2, 0}, {2, 0}},    // Horizontal Line
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


Tetromino CreateTetromino(const std::vector<SDL_Point>& shape, int xOffset, int yOffset) {
    Tetromino t;
    for (const auto& p : shape) {
        t.blocks.push_back({ p.x + xOffset, p.y + yOffset });
    }
    return t;
}

bool Test_Collision() {
    std::vector<Tetromino> placed;

    // Test 1: No placed tetrominos = no collision
    Tetromino test1 = CreateTetromino(shapes[0], SnapToGrid(5,40), SnapToGrid(5, 40));
    assert(CheckCollision(test1, placed) == false);

    // Test 2: Collision — block at (5,5)
    placed.push_back(CreateTetromino(shapes[0], SnapToGrid(5, 40), SnapToGrid(5, 40))); // square at same spot
    Tetromino test2 = CreateTetromino(shapes[1], SnapToGrid(5, 40), SnapToGrid(5, 40)); // long overlapping square
    assert(CheckCollision(test2, placed) == true);

    // Test 3: No collision — test block is far away

    Tetromino test3 = CreateTetromino(shapes[2], SnapToGrid(100, 40), SnapToGrid(10, 40));
    assert(CheckCollision(test3, placed) == false);

    // Test 4: Edge touch but no overlap — no collision
    Tetromino test4 = CreateTetromino(shapes[0], SnapToGrid(7, 40), SnapToGrid(5, 40)); // square to the right of existing one
    assert(CheckCollision(test4, placed) == true);

    std::cout << "All collision tests passed.\n";
    return 0;

}

int Test_Position() {


    tetrominos.clear();

    // Add a shape that *does not* conflict with spawnX/spawnY
    tetrominos.push_back(CreateTetromino(shapes[0], 3, 8)); // out of the way
    assert(IsPositionFree(10) == true);

    // Add a shape that *does* conflict
    tetrominos.clear();
    tetrominos.push_back(CreateTetromino(shapes[6], spawnX, 10)); // Square at spawn
    assert(IsPositionFree(10) == false); // Should overlap

    // Add a Dot that hits spawn exactly
    tetrominos.clear();
    tetrominos.push_back(CreateTetromino(shapes[16], spawnX, 10));
    assert(IsPositionFree(10) == false);

    // Move Dot away — should be free
    tetrominos.clear();
    tetrominos.push_back(CreateTetromino(shapes[16], spawnX + 1, 10));
    assert(IsPositionFree(10) == true);

    std::cout << "All shape-based tests passed.\n";
    return 0;


}

bool Test_InsideGrid() {

    int GRID_START_X = 112;
    int GRID_START_Y = 125;
        int BLOCK_SIZE = 40;
        int Rows = 9;
    // Test 1: Inside grid, near center
    Tetromino t1 = CreateTetromino(shapes[0], 3 * BLOCK_SIZE, 5 * BLOCK_SIZE);
    assert(IsInsideGrid(t1, GRID_START_X, GRID_START_Y) == true);

    // Test 2: Slightly outside left/top (within -5 margin)
    Tetromino t2 = CreateTetromino(shapes[0], SnapToGrid(-4,40), SnapToGrid(-4,40));
    assert(IsInsideGrid(t2, GRID_START_X, GRID_START_Y) == false);

    // Test 3: Fully outside top-left
    Tetromino t3 = CreateTetromino(shapes[0], SnapToGrid(-40, 40), SnapToGrid(-40, 40));
    assert(IsInsideGrid(t3, GRID_START_X, GRID_START_Y) == false);

    // Test 5: Fully outside right edge
    Tetromino t5 = CreateTetromino(shapes[0], SnapToGrid(1000, 40), SnapToGrid(1000, 40));
    assert(IsInsideGrid(t5, GRID_START_X, GRID_START_Y) == false);

    // Test 6: Fully outside bottom edge
    Tetromino t6 = CreateTetromino(shapes[0], SnapToGrid(-1000,40) , SnapToGrid(-1000,40));
    assert(IsInsideGrid(t6, GRID_START_X, GRID_START_Y) == false);

    std::cout << "All IsInsideGrid tests passed.\n";
    return 0;

}