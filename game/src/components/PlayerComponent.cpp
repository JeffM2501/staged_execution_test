#include "components/PlayerComponent.h"
#include "components/TransformComponent.h"
#include "components/BulletComponent.h"

#include "TimeUtils.h"

void PlayerComponent::Update()
{
    auto transform = GetEntityComponent<TransformComponent>();
    if (transform)
    {
        transform->Position += Input * PlayerSpeed * GetDeltaTime();

        if (ShootThisFrame)
        {
            if (GetFrameStartTime() - LastShotTime > ReloadTime)
            {
                LastShotTime = GetFrameStartTime();

                auto bullet = EntitySystem::AddComponent<BulletComponent>(EntitySystem::NewEntityId());
                auto bulletTransform = bullet->AddComponent<TransformComponent>();
                bulletTransform->Position = transform->Position;

                constexpr int spread = 50;
                float speed = PlayerSpeed * 2 + float(GetRandomValue(0, int(PlayerSpeed)));

                bulletTransform->Velocity = Vector2(speed, float(GetRandomValue(-spread, spread))) + (Input * PlayerSpeed);
                EntitySystem::AwakeEntity(bullet->EntityID);
            }
        }
    }

    ShootThisFrame = false;
}