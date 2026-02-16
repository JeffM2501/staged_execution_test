#include "components/NPCComponent.h"
#include "components/TransformComponent.h"

#include "TimeUtils.h"
#include "TaskManager.h"
#include "GameInfo.h"

bool MoveEntity(TransformComponent& entity, float size, Vector2 delta, const BoundingBox2D& bounds)
{
    bool hit = false;

    Vector2 newPos = entity.Position + delta;

    if (newPos.x > bounds.Max.x - size)
    {
        newPos.x = bounds.Max.x - size;
        entity.Velocity.x *= -1;
        hit = true;
    }
    else if (newPos.x < bounds.Min.x + size)
    {
        newPos.x = bounds.Min.x + size;
        entity.Velocity.x *= -1;
        hit = true;
    }

    if (newPos.y > bounds.Max.y - size)
    {
        newPos.y = bounds.Max.y - size;
        entity.Velocity.y *= -1;
        hit = true;
    }
    else if (newPos.y < bounds.Min.y + size)
    {
        newPos.y = bounds.Min.y + size;
        entity.Velocity.y *= -1;
        hit = true;
    }

    entity.Position = newPos;
    return hit;
}

void NPCComponent::Update()
{
    auto transform = GetEntityComponent<TransformComponent>();
    if (transform)
    {
        float delta = TaskManager::GetFixedDeltaTime();
        MoveEntity(*transform, Size, transform->Velocity * delta, WorldBounds.load());
        LastUpdateTime = GetFrameStartTime();
    }
}