#ifndef TEXT_RENDER_HPP
#define TEXT_RENDER_HPP

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class TextRender {
public:
    TextRender(SDL_Renderer* renderer, const std::string& fontPath, int fontSize);
    ~TextRender();

    void updateText(const std::string& newText, SDL_Color color);
    void renderText(int x, int y);


    int getTextWidth() const { return textRect.w; }


    int getTextHeight() const { return textRect.h; }

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* textTexture;
    SDL_Rect textRect;

    void clearTexture();
};

#endif 
