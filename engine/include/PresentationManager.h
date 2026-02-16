#pragma once

#include "raylib.h"

#include <cstddef>
#include <cstdint>
namespace PresentationManager
{
    void Init();

    void Update();

    size_t DefineLayer(uint8_t layer, bool flipOnPresent = true, float frameBufferScale = 1.0f);
    void ReleaseLayer(size_t layer);

    void BeginLayer(size_t layer);
    void EndLayer();

    Rectangle GetCurrentLayerRect();

    void SetLayerAlpha(size_t layer, float alpha);
    void SetLayerTint(size_t layer, Color tint);
    void SetLayerOffset(size_t layer, Vector2 tint);

    void SetLayerShader(size_t layer, Shader shader);
    void ClearLayerShader(size_t layer);

    void Present();
    void Cleanup();
}