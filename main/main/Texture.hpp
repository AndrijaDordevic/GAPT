#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "Tetromino.hpp" // Include the Tetromino header file


extern SDL_Texture* redTexture;
extern SDL_Texture* blueTexture;
extern SDL_Texture* greenTexture;
extern SDL_Texture* yellowTexture;
extern SDL_Texture* orangeTexture;
extern SDL_Texture* purpleTexture;
extern SDL_Texture* MagentaTexture;
extern SDL_Texture* cyanTexture;



SDL_Texture* LoadMenuOptionTextureSelected(SDL_Renderer* ren);
SDL_Texture* LoadGameTexture(SDL_Renderer* ren);
SDL_Texture* LoadMenuTexture(SDL_Renderer* ren);
SDL_Texture* LoadMenuOptionTexture(SDL_Renderer* ren);
bool LoadBlockTextures(SDL_Renderer* ren);
bool IsColorEqual(const SDL_Color& a, const SDL_Color& b);
SDL_Texture* ApplyTexture(Block block);



#endif // TEXTURE_HPP