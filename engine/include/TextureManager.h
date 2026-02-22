#pragma once

#include "raylib.h"

#include <memory>
#include <atomic>


// manages texture lifecycle
// TODO, add smart material manager too

namespace TextureManager
{
    struct TextureInfo
    {
        Texture ID = { 0 };
        std::atomic_bool Ready = false;
        Rectangle Bounds = { 0,0,0,0 };
    };
    
    using TextureReference = std::shared_ptr<TextureInfo>;

    void Init();
    void Shutdown();

    void Update();

    TextureReference GetTexture(size_t hash);
}