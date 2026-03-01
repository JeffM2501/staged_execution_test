#include "components/NPCSpawnComponent.h"
#include "components/NPCComponent.h"
#include "components/TransformComponent.h"

#include "TimeUtils.h"
#include "GameInfo.h"

void NPCSpawnComponent::OnAwake()
{
    NextSpawnInterval = float(GetRandomValue(int(MinInterval * 1000), int(MaxInterval * 1000))) / 1000.0f;

    for (size_t i = 0; i < MaxSpawnCount; i++)
    {
        PrefabReader.ReadEntitiesFromResource(NPCPrefab, [this](std::span<size_t> entities)
            {
                float size = float(GetRandomValue(10, 30));

                auto npcTransform = EntitySystem::GetEntityComponent<TransformComponent>(entities[0]);
                if (npcTransform)
                {
                    npcTransform->Position = GetRandomPosInBounds(WorldBounds, size);
                    constexpr int spread = 50;
                    float velocity = float(GetRandomValue(int(MinVelocity * 1000), int(MinVelocity * 1000))) / 1000.0f;

                    npcTransform->Velocity = GetRandomVector(velocity);
                }

                auto npc = EntitySystem::GetEntityComponent<NPCComponent>(entities[0]);
                if (npc)
                {
                    npc->Size = size;
                    npc->Tint = Color{ uint8_t(GetRandomValue(32, 64)), uint8_t(GetRandomValue(0, 32)), uint8_t(GetRandomValue(128, 255)), 255 };
                }
            });
    }
}

void NPCSpawnComponent::Update()
{

    
}
