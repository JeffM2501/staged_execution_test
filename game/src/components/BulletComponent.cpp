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

    auto transform = GetEntityComponent<TransformComponent>().Get();
    if (transform)
    {
        transform->Position += transform->Velocity * GetDeltaTime();
    }

    Sprite.Rotation += 500 * GetDeltaTime() * SpinDir;
    Sprite.Rotation = fmodf(Sprite.Rotation, 360);
}

void BulletComponent::OnAwake()
{
    SpinDir = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;
}
