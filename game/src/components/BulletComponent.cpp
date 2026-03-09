#include "components/BulletComponent.h"
#include "components/TransformComponent.h"

#include "TimeUtils.h"

#include "raylib.h"
#include "raymath.h"

void BulletComponent::Update()
{
    Lifetime -= GetDeltaTime();
    if (Lifetime < 0)
    {
        EntitySystem::RemoveEntity(EntityID);
        return;
    }

    auto transform = GetEntityComponent<TransformComponent>();
    if (transform)
    {
        transform->Position += transform->Velocity * GetDeltaTime();
    }

    Sprite.Rotation += 1000 * GetDeltaTime() * SpinDir;
    Sprite.Rotation = fmodf(Sprite.Rotation, 360);
}

void BulletComponent::OnAwake()
{
    SpinDir = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
}

bool BulletComponent::OnDataRead(BufferReader& buffer)
{
    Size = buffer.Read<float>();
    Damage = buffer.Read<float>();
    Lifetime = buffer.Read<float>();
    Tint = buffer.ReadColor();

    Sprite = SpriteManager::LoadFromBuffer(buffer);

    TraceLog(LOG_INFO, "Loaded BulletComponent for entity %zu", EntityID);
    return true;
}
