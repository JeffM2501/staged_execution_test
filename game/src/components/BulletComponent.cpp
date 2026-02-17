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
}