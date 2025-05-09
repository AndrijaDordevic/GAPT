#pragma once
#include <SDL3/SDL.h>
#include <cstdlib>

struct ScreenShake {
    float duration = 0.0f;
    float magnitude = 0.0f;

    void start(float seconds, float maxOffset) {
        duration = seconds;
        magnitude = maxOffset;
    }

    SDL_FPoint update(float deltaTime) {
        if (duration <= 0.0f) return { 0, 0 };
        duration -= deltaTime;
        float currentMag = magnitude * (duration / (duration + deltaTime));
        float dx = ((rand() / (float)RAND_MAX) * 2 - 1) * currentMag;
        float dy = ((rand() / (float)RAND_MAX) * 2 - 1) * currentMag;
        return { dx, dy };
    }

    bool active() const { return duration > 0.0f; }
};
