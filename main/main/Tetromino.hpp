#ifndef TETROMINO_HPP
#define TETROMINO_HPP

#include <vector>
#include <SDL3/SDL.h>

// Forward declaration of Block
struct Block;

const int BLOCK_SIZE = 80;
const int Columns = 9;
extern bool draggingInProgress;
extern bool CanDrag;

struct Block {
    int x, y;
    SDL_Color color;
    bool dragging = false;
    SDL_Texture* texture = nullptr;
};

struct Tetromino {
    std::vector<Block> blocks;
    SDL_Color color;
    bool canBeDragged = true;
    int layer = 0;
};

// Change from pointers to objects:
extern std::vector<Tetromino> tetrominos;
extern std::vector<Tetromino> placedTetrominos;


extern int spawnX;
extern int spawnYPositions[3];
extern bool positionsOccupied[3];
extern int spawnedCount;
extern int score;

// Change CheckCollision signature:
bool IsPositionFree(int spawnY);
void SpawnTetromino();
void ReleaseOccupiedPositions();
void RenderTetrominos(SDL_Renderer* ren);
int SnapToGrid(int value, int gridStart);
bool CheckCollision(const Tetromino& tetro, const std::vector<Tetromino>& placedTetrominos);
bool IsInsideGrid(const Tetromino& tetro, int gridStartX, int gridStartY);
void RunBlocks(SDL_Renderer* renderer);
void AddToIndividualBlocks(const Tetromino& tetro);
void ClearSpanningTetrominos(int gridStartX, int gridStartY, int gridCols, int gridRows);
bool LoadBlockTextures(SDL_Renderer* ren);
bool IsColorEqual(const SDL_Color& a, const SDL_Color& b);
void RenderScore(SDL_Renderer* renderer, int score);

#endif // TETROMINO_HPP
