#include "SpriteManager.h"
#include "ResourceManager.h"
#include "BufferReader.h"

#include <unordered_map>

namespace SpriteManager
{
    std::unordered_map<size_t, SpriteReference> Sprites;

    void Sprite::Draw(size_t frame, Vector2 position, float scale, float rotation, Color tint)
    {
        if (!Ready.load(std::memory_order_acquire))
            return;

        auto it = Frames.find(frame);
        if (it == Frames.end())
            return;

        Rectangle sourceRect = Rectangle{ it->second.x * Texture->ID.width, it->second.y * Texture->ID.height, it->second.width * Texture->ID.width, it->second.height * Texture->ID.height };
        Rectangle destRect = { position.x, position.y, sourceRect.width * scale, sourceRect.height * scale };
        DrawTexturePro(Texture->ID, sourceRect, destRect, { destRect.width * 0.5f, destRect.height * 0.5f }, rotation, tint);
    }

    void SpriteInstance::Draw(Vector2 position, Color tint)
    {
        SpriteRef->Draw(CurrentFrame, position, Scale, Rotation, tint);
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

                reader.Read<uint32_t>();
                reader.Read<uint32_t>();

                size_t textureHash = reader.Read<size_t>();

                sprite->Texture = TextureManager::GetTexture(textureHash);
                uint32_t frameCount = reader.Read<uint32_t>();
                for (uint32_t i = 0; i < frameCount; i++)
                {
                    Rectangle frameRect = { reader.Read<float>(), reader.Read<float>(), reader.Read<float>(), reader.Read<float>() };
                    sprite->Frames[i] = frameRect;
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

    SpriteInstance LoadFromBuffer(BufferReader& buffer)
    {
        auto hash = buffer.Read<size_t>();
        SpriteInstance instance = LoadResoruce(hash);
        instance.CurrentFrame = buffer.Read<uint32_t>();
        instance.Rotation = buffer.Read<float>();

        return instance;
    }
}