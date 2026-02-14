#include "PresentationManager.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <vector>
#include <unordered_map>
#include <memory>

namespace PresentationManager
{
    struct RenderLayer
    {
        size_t ID = 0;
        uint8_t Order = 0;
        RenderTexture Framebuffer = { 0 };
        bool DrawnThisFrame = false;
        bool FlipOnPresent = true;
        float FrameBufferScale = 1.0f;

        Shader LayerShader = { 0 };

        float Alpha = 1.0f;
        Color Tint = WHITE;
        Vector2 Offset = Vector2Zeros;

        void Generate()
        {
            if (IsRenderTextureValid(Framebuffer))
                UnloadRenderTexture(Framebuffer);

            Framebuffer = LoadRenderTexture(int(GetScreenWidth() * FrameBufferScale), int(GetScreenHeight() * FrameBufferScale));
        }
    };

    static std::vector<std::unique_ptr<RenderLayer>> Layers;
    static std::unordered_map<size_t, RenderLayer*> LayerIdToIndex;

    RenderLayer* ActiveLayer = nullptr;

    static size_t NextLayerId = 1;

    void Init()
    {
        // Initialize presentation manager resources
    }

    void Update()
    {
        for (auto& layer : Layers)
        {
            layer->DrawnThisFrame = false;
            if (IsWindowResized())
            {
                // Resize the framebuffer of the layer to match the new window size
                layer->Generate();
            }
        }
    }

    size_t DefineLayer(uint8_t order, bool flipOnPresent, float frameBufferScale)
    {
        auto addFunc = [flipOnPresent, frameBufferScale](uint8_t order)
            { 
                size_t newId = NextLayerId++;
                auto newLayer = std::make_unique<RenderLayer>();
                newLayer->ID = newId;
                newLayer->Order = order;
                newLayer->FlipOnPresent = flipOnPresent;
                newLayer->FrameBufferScale = frameBufferScale;
                newLayer->Generate();
                LayerIdToIndex[newId] = newLayer.get();
                return newLayer;
            };

        for (auto itr = Layers.begin(); itr != Layers.end(); ++itr)
        {
            if ((*itr)->Order > order)
            {
                auto layer = addFunc(order);
                size_t newId = layer->ID;
                Layers.insert(itr, std::move(layer));
                return newId;
            }
        }

        auto layer = addFunc(order);
        size_t newId = layer->ID;
        Layers.push_back(std::move(layer));
        return newId;
    }

    void ReleaseLayer(size_t layer)
    {
        auto itr = LayerIdToIndex.find(layer);
        if (itr == LayerIdToIndex.end())
        {
            // Invalid layer ID
            return;
        }

        UnloadRenderTexture(itr->second->Framebuffer);
        auto layerItr = std::find_if(Layers.begin(), Layers.end(), [itr](const std::unique_ptr<RenderLayer>& l)
        {
            return l.get() == itr->second;
            });

        Layers.erase(layerItr);
        LayerIdToIndex.erase(itr);
    }

    void BeginLayer(size_t layer)
    {
        if (ActiveLayer)
            EndTextureMode();

        ActiveLayer = nullptr;

        auto itr = LayerIdToIndex.find(layer);
        if (itr == LayerIdToIndex.end())
        {
            // Invalid layer ID
            return;
        }

        ActiveLayer = itr->second;
        ActiveLayer->DrawnThisFrame = true;
        BeginTextureMode(ActiveLayer->Framebuffer);
        ClearBackground(BLANK);
    }

    void EndLayer()
    {
        if (ActiveLayer)
            EndTextureMode();

        ActiveLayer = nullptr;
    }

    void SetLayerAlpha(size_t layer, float alpha)
    {
        auto itr = LayerIdToIndex.find(layer);
        if (itr == LayerIdToIndex.end())
        {
            // Invalid layer ID
            return;
        }
        itr->second->Alpha = alpha;
    }

    void SetLayerTint(size_t layer, Color tint)
    {
        auto itr = LayerIdToIndex.find(layer);
        if (itr == LayerIdToIndex.end())
        {
            // Invalid layer ID
            return;
        }
        itr->second->Tint = tint;
    }

    void SetLayerOffset(size_t layer, Vector2 offset)
    {
        auto itr = LayerIdToIndex.find(layer);
        if (itr == LayerIdToIndex.end())
        {
            // Invalid layer ID
            return;
        }
        itr->second->Offset = offset;
    }

    void SetLayerShader(size_t layer, Shader shader)
    {
        auto itr = LayerIdToIndex.find(layer);
        if (itr == LayerIdToIndex.end())
        {
            // Invalid layer ID
            return;
        }
        itr->second->LayerShader = shader;
    }

    void ClearLayerShader(size_t layer)
    {
        auto itr = LayerIdToIndex.find(layer);
        if (itr == LayerIdToIndex.end())
        {
            // Invalid layer ID
            return;
        }
        itr->second->LayerShader.id = rlGetShaderIdDefault();
        itr->second->LayerShader.locs = rlGetShaderLocsDefault();
     }

    void Present()
    {
        EndLayer();

        Rectangle destRect = { 0, 0, float(GetScreenWidth()), float(GetScreenHeight()) };
        for (auto& layer : Layers)
        {
            if (!layer->DrawnThisFrame)
                continue;

            Rectangle sourceRect = { 0, 0, float(layer->Framebuffer.texture.width), float(layer->Framebuffer.texture.height) };

            if (layer->FlipOnPresent)
                sourceRect.height *= -1;

            if (layer->LayerShader.id != 0)
                BeginShaderMode(layer->LayerShader);
            DrawTexturePro(layer->Framebuffer.texture, sourceRect, destRect, layer->Offset * -1, 0, ColorAlpha(layer->Tint, layer->Alpha));

            if (layer->LayerShader.id != 0)
                EndShaderMode();
        }
    }

    void Cleanup()
    {
        for (auto& layer : Layers)
        {
            if (IsRenderTextureValid(layer->Framebuffer))
                UnloadRenderTexture(layer->Framebuffer);
        }

        Layers.clear();
        LayerIdToIndex.clear();
    }
}