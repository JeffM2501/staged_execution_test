#include "SpriteManager.h"
#include "ResourceManager.h"
#include "BufferReader.h"

#include <unordered_map>

namespace SpriteManager
{
    std::unordered_map<size_t, SpriteReference> Sprites;

    void Sprite::Draw(size_t frame, Vector2 position, float scale, Color tint)
    {
        if (!Ready.load(std::memory_order_acquire))
            return;
        auto it = Frames.find(frame);
        if (it == Frames.end())
            return;
        Rectangle sourceRect = it->second;
        DrawTexturePro(Texture->ID, sourceRect, { position.x, position.y, sourceRect.width * scale, sourceRect.height * scale }, { 0,0 }, 0.0f, tint);
    }

    void SpriteInstance::Draw(Vector2 position, Color tint)
    {
        SpriteRef->Draw(CurrentFrame, position, 1.0f, tint);
    }

    SpriteInstance SpriteInstance::Clone()
    {
        return InstanceFromSpite(SpriteRef);
    }

    SpriteInstance LoadResoruce(size_t hash)
    {
        if(Sprites.contains(hash))
            return InstanceFromSpite(Sprites[hash]);

        SpriteReference sprite = std::make_shared<Sprite>();
        ResourceManager::LoadResource(hash, ResourceManager::ResourceType::File, [sprite](const ResourceManager::ResourceInfoRef& data)
            {
                // TODO, parse sprite data
                BufferReader reader(std::get<std::vector<unsigned char>>(data->Data));
                size_t textureHash = reader.Read<size_t>();

                sprite->Texture = TextureManager::GetTexture(textureHash);
                size_t frameCount = reader.Read<size_t>();
                for (size_t i = 0; i < frameCount; i++)
                {
                    size_t frameHash = reader.Read<size_t>();
                    Rectangle frameRect = { reader.Read<float>(), reader.Read<float>(), reader.Read<float>(), reader.Read<float>() };
                    sprite->Frames[frameHash] = frameRect;
                }
                sprite->Ready.store(true);
            });

        Sprites.insert({ hash, sprite });
        return InstanceFromSpite(sprite);
    }

    SpriteInstance InstanceFromSpite(SpriteReference ref)
    {
        SpriteInstance instance;
        instance.SpriteRef = ref;
        return instance;
    }
}