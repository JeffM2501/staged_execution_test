#include "components/PlayerComponent.h"
#include "components/TransformComponent.h"
#include "GameInfo.h"

#include "TimeUtils.h"

void PlayerComponent::OnAwake()
{
}

void PlayerComponent::Update()
{
    auto transform = GetEntityComponent<TransformComponent>().Get();
    if (transform)
    {
        transform->Position += Input * PlayerSpeed * GetDeltaTime();

        LastShotTime += GetDeltaTime();
        if (ShootThisFrame)
        {
            if (LastShotTime > ReloadTime)
            {
                LastShotTime = 0;;

                Vector2 pos = transform->Position;

                PrefabReader.ReadEntitiesFromResource(BulletPrefab, [pos, this](std::span<size_t> entities)
                    {
                        auto bulletTransform = EntitySystem::GetEntityComponent<TransformComponent>(entities[0]);
                        if (bulletTransform)
                        {
                            bulletTransform->Position = pos;
                            constexpr int spread = 50;
                            float speed = PlayerSpeed * 3 + float(GetRandomValue(0, int(PlayerSpeed)));

                            bulletTransform->Velocity = Vector2(speed, float(GetRandomValue(-spread, spread))) + (Input * PlayerSpeed);
                        }
                    });
            }
        }
    }

    ShootThisFrame = false;
}