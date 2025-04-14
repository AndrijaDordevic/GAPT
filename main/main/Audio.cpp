#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Audio.hpp"
#include <iostream>
#include <thread>
#include <chrono>

static ma_engine engine;
static bool engineInitialized = false;

namespace Audio {

    bool Init() {
        if (engineInitialized) return true;

        if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to initialize engine\n";
            return false;
        }

        engineInitialized = true;
        return true;
    }

    void PlaySoundFile(const std::string& filepath) {
        if (!engineInitialized && !Init()) return;

        if (ma_engine_play_sound(&engine, filepath.c_str(), NULL) != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to play: " << filepath << "\n";
        }
    }

    void Shutdown() {
        if (engineInitialized) {
            ma_engine_uninit(&engine);
            engineInitialized = false;
        }
    }

} // namespace Audio
