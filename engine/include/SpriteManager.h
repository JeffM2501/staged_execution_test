#pragma once

#include "TextureManager.h"

#include <memory>
#include <unordered_map>
#include <atomic>

namespace SpriteManager
{
    class Sprite
    {
    public:
        TextureManager::TextureReference Texture;
        std::unordered_map<size_t, Rectangle> Frames;

        std::atomic_bool Ready = false;

        void Draw(size_t frame, Vector2 position, float scale, Color tint = WHITE);
    };

    using SpriteReference = std::shared_ptr<Sprite>;

    class SpriteInstance
    {
    public:
        SpriteReference SpriteRef;
        size_t CurrentFrame = 0;

        void Draw(Vector2 position, Color tint = WHITE);

        SpriteInstance Clone();
    };

    SpriteInstance LoadResoruce(size_t hash);
    SpriteInstance InstanceFromSpite(SpriteReference ref);
}