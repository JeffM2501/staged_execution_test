#include "components/PlayerComponent.h"
#include "components/TransformComponent.h"
#include "GameInfo.h"

#include "TimeUtils.h"

void PlayerComponent::OnAwake()
{
   // Transform.Set(GetEntityComponent<TransformComponent>());
}

bool PlayerComponent::OnDataRead(BufferReader& buffer)
{
    Size = buffer.Read<float>();
    Health = buffer.Read<float>();
    PlayerSpeed = buffer.Read<float>();
    ReloadTime = buffer.Read<float>();
    BulletPrefab = buffer.Read<size_t>();
    ShotSpread = buffer.Read<float>();
    ShotSpeedMultiplyer = buffer.Read<float>();
    ShotSpeedVariance = buffer.Read<float>();

    Sprite = SpriteManager::LoadFromBuffer(buffer);

    TraceLog(LOG_INFO, "Loaded PlayerComponent for entity %zu", EntityID);
    return true;
}

void PlayerComponent::Update()
{
    auto transform = GetEntityComponent<TransformComponent>();
    {
        transform->Position += Input * PlayerSpeed * GetDeltaTime();

        LastShotTime += GetDeltaTime();
        if (ShootThisFrame)
        {
            if (LastShotTime > ReloadTime)
            {
                LastShotTime = 0;

                Vector2 pos = transform->Position;

                PrefabReader.ReadEntitiesFromResource(BulletPrefab, [pos, this](std::span<size_t> entities)
                    {
                        auto bulletTransform = EntitySystem::GetEntityComponent<TransformComponent>(entities[0]);
                        if (bulletTransform)
                        {
                            bulletTransform->Position = pos;
                            float speed = PlayerSpeed * ShotSpeedMultiplyer + float(GetRandomValue(0, int(PlayerSpeed * ShotSpeedVariance)));

                            bulletTransform->Velocity = Vector2(speed, float(GetRandomValue(int(-ShotSpread), int(ShotSpread)))) + (Input * PlayerSpeed);
                        }
                    });
            }
        }
    }

    ShootThisFrame = false;
}