#include <SDL3/SDL.h>
#include "Window.hpp"
#include "Audio.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <memory>
#include "Tetromino.hpp"
#include "Texture.hpp"
#include <SDL3_image/SDL_image.h>
#include "TextRender.hpp"
#include "Client.hpp"
#include "Shapes.hpp"
// Global flags and spawn data
bool draggingInProgress = false;
bool CanDrag = false;
bool positionsOccupied[3] = { false, false, false };
bool searched = false;


int spawnX = 0;
int spawnYPositions[3] = { 0, 0, 0 };
int spawnedCount = 0;
int score = 0;
int OpponentScore = 0;

const int POINTS_PER_LINE = 100;



// Store tetrominos by value instead of pointers.
// Make sure these are defined only once in your project.
std::vector<Tetromino> tetrominos;         // Active tetrominos (for drag, etc.)
std::vector<Tetromino> placedTetrominos;     // Tetrominos that have been placed

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
		{0, 255, 255, 255},  // Cyan
		{128, 0, 128, 255},  // Purple
		{255, 0, 255, 255},  // Magenta

	};

	//Creating a random shape/Color from the vector
	int randomColorIndex = rand() % (sizeof(colors) / sizeof(colors[0]));
	int randomShapeIndex = Client::shape.front();

	//Creaging tetromino object and assiging a color
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

	for (auto& block : newTetromino.blocks) {

		block.color = newTetromino.color;

		//The texture returned from the ApplyTexture function is assigned to a texture variable
		SDL_Texture* textureChosen = ApplyTexture(block);

		//Applies the texture
		block.texture = textureChosen;

	}

	// Add the new tetromino to the active container.
	std::cout << "Tetromino spawned at (" << spawnX << ", " << chosenSpawnY << ")\n";
	tetrominos.push_back(newTetromino);
	spawnedCount++;
	Client::shape.erase(Client::shape.begin());
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

	// Sort tetrominos by their layer value.
	std::sort(allTetrominos.begin(), allTetrominos.end(), [](const Tetromino& a, const Tetromino& b) {
		return a.layer < b.layer;
		});

	// Render each tetromino's blocks
	for (const auto& tetrominoRef : allTetrominos) {
		const Tetromino& tetromino = tetrominoRef.get();
		for (auto& block : tetromino.blocks) {
			SDL_FRect rect = {
				static_cast<float>(block.x),
				static_cast<float>(block.y),
				static_cast<float>(BLOCK_SIZE),
				static_cast<float>(BLOCK_SIZE)
			};

			if (block.texture) {
				SDL_RenderTexture(ren, block.texture, nullptr, &rect);

			}
			else {
				// Fallback: Color if no texture
				SDL_SetRenderDrawColor(ren, block.color.r, block.color.g, block.color.b, block.color.a);
				SDL_RenderFillRect(ren, &rect);
			}

		}
	}
}


// Snap value to nearest multiple of BLOCK_SIZE
int SnapToGrid(int value, int gridStart) {
	return gridStart + ((value - gridStart + BLOCK_SIZE / 2) / BLOCK_SIZE) * BLOCK_SIZE;
}

// Check if a tetromino collides with placed tetrominos or individual blocks.
bool CheckCollision(const Tetromino& tetro, const std::vector<Tetromino>& placedTetrominos) {
	// Check collision with each tetromino in the placedTetrominos vector.
	for (const auto& placed : placedTetrominos) {
		for (const auto& placedBlock : placed.blocks) {
			for (const auto& block : tetro.blocks) {
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
		//the -5 and -20 is so the blocks can be placed slightly outside the grid using the cursor.
		if (block.x < gridStartX - 5 || block.y < gridStartY - 5 ||
			block.x > gridStartX + (Columns * BLOCK_SIZE) - 20 ||
			block.y > gridStartY + (Columns * BLOCK_SIZE) - 20)
		{
			return false;
		}
	}
	return true;
}

void RenderScore(SDL_Renderer* renderer, int localScore, int opponentScore) {
    TextRender playerscoreText(renderer, "assets/Arial.ttf", 28);
	TextRender opponentscoreText(renderer, "assets/Arial.ttf", 28);
    SDL_Color white = { 255, 255, 255, 255 };

	std::string PlayerScore = std::to_string(localScore);
	std::string EnemyScore = std::to_string(opponentScore);
    playerscoreText.updateText(PlayerScore, white);
	opponentscoreText.updateText(EnemyScore, white);
    playerscoreText.renderText(300, 47);
	opponentscoreText.renderText(540,890);
}



// This function is called in your main loop to handle tetromino spawning.
void RunBlocks(SDL_Renderer* renderer) {
	static bool initialized = false;

	// Set spawn positions
	spawnX = WINDOW_WIDTH - BLOCK_SIZE - 270;
	spawnYPositions[0] = 90;
	spawnYPositions[1] = 375;
	spawnYPositions[2] = 630;

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

	// Mark occupancy from placed tetrominos.
	markOccupancy(placedTetrominos);

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

	// Build vectors of cleared row and column indices
	std::vector<int> clearedRowsVec, clearedColsVec;
	for (int r = 0; r < gridRows; ++r)
		if (completeRows[r]) clearedRowsVec.push_back(r);

	for (int c = 0; c < gridCols; ++c)
		if (completeCols[c]) clearedColsVec.push_back(c);

	int serverScore = 0;

	std::cout << "clearedRowsVec size: " << clearedRowsVec.size() << "\n";
	std::cout << "clearedColsVec size: " << clearedColsVec.size() << "\n";

	if (!clearedRowsVec.empty() || !clearedColsVec.empty()) {
		std::cout << "Sending score request...\n";
		serverScore = Client::sendClearedLinesAndGetScore(clearedRowsVec, clearedColsVec);
		score = serverScore;
		SDL_Delay(100); // Optional delay to allow server to process
	}

	std::cout << "Score (from server): " << serverScore << " | Total: " << score << std::endl;


	// Remove cleared blocks from the placed tetrominos.
	bool anyBlocksCleared = false;
	auto removeClearedBlocks = [&](std::vector<Tetromino>& container) {
		for (auto& tetro : container) {
			auto originalSize = tetro.blocks.size();
			auto it = std::remove_if(tetro.blocks.begin(), tetro.blocks.end(),
				[&](const Block& block) {
					int col = (block.x - gridStartX) / BLOCK_SIZE;
					int row = (block.y - gridStartY) / BLOCK_SIZE;
					bool removeRow = (row >= 0 && row < gridRows && completeRows[row]);
					bool removeCol = (col >= 0 && col < gridCols && completeCols[col]);
					return removeRow || removeCol;
				});
			tetro.blocks.erase(it, tetro.blocks.end());
			if (tetro.blocks.size() < originalSize) {
				anyBlocksCleared = true;
			}
		}
		container.erase(std::remove_if(container.begin(), container.end(),
			[](const Tetromino& t) { return t.blocks.empty(); }),
			container.end());
		};

	// Run the clearing logic
	removeClearedBlocks(placedTetrominos);

	// ✅ Play sound **only if blocks were actually cleared**
	if (anyBlocksCleared) {
		Audio::PlaySoundFile("Assets/Sounds/BlocksPop.mp3");
	}

}


void RecieveOpponentScore(int OpScore) {
	OpponentScore = OpScore;
}