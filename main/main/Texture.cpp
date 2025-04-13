#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>
#include "Tetromino.hpp"
#include "Texture.hpp"

//Defines all the Textures
SDL_Texture* redTexture = nullptr;
SDL_Texture* blueTexture = nullptr;
SDL_Texture* greenTexture = nullptr;
SDL_Texture* yellowTexture = nullptr;
SDL_Texture* orangeTexture = nullptr;
SDL_Texture* purpleTexture = nullptr;
SDL_Texture* MagentaTexture = nullptr;
SDL_Texture* cyanTexture = nullptr;


SDL_Texture* LoadGameTexture(SDL_Renderer* ren) {

	const char* path = "Assets/GameUI.png";

	SDL_Texture* texture = IMG_LoadTexture(ren, path);
	if (!texture) {
		printf("Create Texture failed: %s\n", SDL_GetError());
	}

	return texture;

}
SDL_Texture* LoadClearGridTextureS(SDL_Renderer* ren) {

	const char* path = "Assets/ClearGridSelected.png";

	SDL_Texture* texture = IMG_LoadTexture(ren, path);
	if (!texture) {
		printf("Create Texture failed: %s\n", SDL_GetError());
	}

	return texture;

}

SDL_Texture* LoadClearGridTexture(SDL_Renderer* ren) {

	const char* path = "Assets/ClearGrid.png";

	SDL_Texture* texture = IMG_LoadTexture(ren, path);
	if (!texture) {
		printf("Create Texture failed: %s\n", SDL_GetError());
	}

	return texture;

}
SDL_Texture* LoadMenuTexture(SDL_Renderer* ren) {

	const char* path = "Assets/Menu.png";

	SDL_Texture* texture = IMG_LoadTexture(ren, path);
	if (!texture) {
		printf("Create Texture failed: %s\n", SDL_GetError());
	}

	return texture;

}

SDL_Texture* LoadMenuOptionTexture(SDL_Renderer* ren) {

	const char* path = "Assets/MenuOption.png";

	SDL_Texture* texture = IMG_LoadTexture(ren, path);
	if (!texture) {
		printf("Create Texture failed: %s\n", SDL_GetError());
	}
	

	return texture;

}

SDL_Texture* LoadMenuOptionTextureSelected(SDL_Renderer* ren) {

	const char* path = "Assets/MenuOptionS.png";

	SDL_Texture* textureS = IMG_LoadTexture(ren, path);
	if (!textureS) {
		printf("Create Texture failed: %s\n", SDL_GetError());
	}

	return textureS;

}

//Loads all the textures
bool LoadBlockTextures(SDL_Renderer* ren) {
	redTexture = IMG_LoadTexture(ren, "Assets/BlockRed.png");
	blueTexture = IMG_LoadTexture(ren, "Assets/BlockBlue.png");
	greenTexture = IMG_LoadTexture(ren, "Assets/BlockGreen.png");
	yellowTexture = IMG_LoadTexture(ren, "Assets/BlockYellow.png");
	orangeTexture = IMG_LoadTexture(ren, "Assets/BlockOrange.png");
	cyanTexture = IMG_LoadTexture(ren, "Assets/BlockCyan.png");
	purpleTexture = IMG_LoadTexture(ren, "Assets/BlockPurple.png");
	MagentaTexture = IMG_LoadTexture(ren, "Assets/BlockMagenta.png");


	return (redTexture && blueTexture && greenTexture && yellowTexture && orangeTexture && cyanTexture && purpleTexture && MagentaTexture);
}


//Compares two colors
bool IsColorEqual(const SDL_Color& a, const SDL_Color& b) {
	return (a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
}


//returns the texture of the block depending on the color
SDL_Texture* ApplyTexture(Block block) {

	if (IsColorEqual(block.color, { 255,0,0,255 })) {
		return  redTexture;
	}
	else if (IsColorEqual(block.color, { 0, 0, 255, 255 })) {
		return blueTexture;
	}
	else if (IsColorEqual(block.color, { 0, 255, 0, 255 })) {
		return greenTexture;
	}
	else if (IsColorEqual(block.color, { 255, 255, 0, 255 })) {
		return yellowTexture;
	}
	else if (IsColorEqual(block.color, { 255, 165, 0, 255 })) {
		return orangeTexture;
	}
	else  if (IsColorEqual(block.color, { 0, 255, 255, 255 })) {
		return cyanTexture;
	}
	else if (IsColorEqual(block.color, { 128, 0, 128, 255 })) {
		return purpleTexture;
	}
	else if (IsColorEqual(block.color, { 255, 0, 255, 255 })) {
		return MagentaTexture;
	}
}