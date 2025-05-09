#pragma once
#include <string>

namespace Audio {
    bool Init();
    void PlaySoundFile(const std::string& filepath);
    void Shutdown();
}
