#include "TextRender.hpp"
#include <iostream>

TextRender::TextRender(SDL_Renderer* renderer, const std::string& fontPath, int fontSize)
    : renderer(renderer), textTexture(nullptr) {
    if (TTF_Init() == -1) {
        std::cerr << "Failed to initialize TTF." << std::endl;
    }
    font = TTF_OpenFont("Assets\\Arial.ttf", fontSize);
    if (!font) {
        std::cerr << "Failed to load font." << std::endl;
    }
}

TextRender::~TextRender() {
    clearTexture();
    TTF_CloseFont(font);
    TTF_Quit();
}

void TextRender::clearTexture() {
    if (textTexture) {
        SDL_DestroyTexture(textTexture);
        textTexture = nullptr;
    }
}

void TextRender::updateText(const std::string& newText, SDL_Color color) {
    clearTexture();

    // Correct usage of SDL_Color as the third argument
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, newText.c_str(), 10, color);
    if (!textSurface) {
        std::cerr << "Text Surface Error." << std::endl;
        return;
    }

    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = { 0, 0, textSurface->w, textSurface->h };
    SDL_DestroySurface(textSurface);
}

void TextRender::renderText(int x, int y) {
    if (textTexture) {
        textRect.x = x;
        textRect.y = y;

        // Convert SDL_Rect to SDL_FRect
        SDL_FRect textFRect = {
            static_cast<float>(textRect.x),
            static_cast<float>(textRect.y),
            static_cast<float>(textRect.w),
            static_cast<float>(textRect.h)
        };

        SDL_RenderTexture(renderer, textTexture, nullptr, &textFRect);
    }
}

