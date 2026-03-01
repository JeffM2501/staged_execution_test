#include "PlayerSpawnComponent.h"
#include "components/TransformComponent.h"
#include "GameInfo.h"

void PlayerSpawnComponent::OnAwake()
{
    auto transform = GetEntityComponent<TransformComponent>();
    if (transform)
    {
        Vector2 pos = transform->Position;

        PrefabReader.ReadEntitiesFromResource(PlayerPrefab, [pos](std::span<size_t> entities)
            {
                EntitySystem::GetEntityComponent<TransformComponent>(entities[0])->Position = pos;
            });
    }
}
