#include "ComponentReader.h"

#include "components/TransformComponent.h"
#include "components/PlayerComponent.h"
#include "components/NPCComponent.h"
#include "components/BulletComponent.h"
#include "components/PlayerSpawnComponent.h"
#include "components/NPCSpawnComponent.h"

void ComponentReader::OnComponentData(EntitySystem::EntityComponent* component, size_t componentId, EntityReader::BufferReader& buffer)
{
    // find the deserializer and call it
    if (componentId == TransformComponent::GetComponentId())
    {
        auto transform = static_cast<TransformComponent*>(component);
        transform->Position.x = buffer.Read<float>();
        transform->Position.y = buffer.Read<float>();
        transform->Velocity.x = buffer.Read<float>();
        transform->Velocity.y = buffer.Read<float>();

        TraceLog(LOG_INFO, "Loaded Transform for entity %zu", component->EntityID);
    }
    else if (componentId == PlayerComponent::GetComponentId())
    {
        auto player = static_cast<PlayerComponent*>(component);
        player->Size = buffer.Read<float>();
        player->Health = buffer.Read<float>();
        player->PlayerSpeed = buffer.Read<float>();
        player->ReloadTime = buffer.Read<float>();
        player->BulletPrefab = buffer.Read<size_t>();

        TraceLog(LOG_INFO, "Loaded PlayerComponent for entity %zu", component->EntityID);
    }
    else if (componentId == NPCComponent::GetComponentId())
    {
        auto npc = static_cast<NPCComponent*>(component);
        npc->Size = buffer.Read<float>();
        npc->Tint = buffer.ReadColor();

        TraceLog(LOG_INFO, "Loaded NPCComponent for entity %zu", component->EntityID);
    }
    else if (componentId == BulletComponent::GetComponentId())
    {
        auto bullet = static_cast<BulletComponent*>(component);
        bullet->Size = buffer.Read<float>();
        bullet->Damage = buffer.Read<float>();
        bullet->Lifetime = buffer.Read<float>();
        bullet->Tint = buffer.ReadColor();

        TraceLog(LOG_INFO, "Loaded BulletComponent for entity %zu", component->EntityID);
    }
    else if (componentId == PlayerSpawnComponent::GetComponentId())
    {
        auto spawn = static_cast<PlayerSpawnComponent*>(component);

        spawn->PlayerPrefab = buffer.Read<size_t>();

        TraceLog(LOG_INFO, "Loaded PlayerSpawnComponent for entity %zu", component->EntityID);
    }
    else if (componentId == NPCSpawnComponent::GetComponentId())
    {
        auto spawn = static_cast<NPCSpawnComponent*>(component);
        spawn->MinInterval = buffer.Read<float>();
        spawn->MaxInterval = buffer.Read<float>();
        spawn->MinVelocity = buffer.Read<float>();
        spawn->MaxVelocity = buffer.Read<float>();
        spawn->MaxSpawnCount = buffer.Read<uint32_t>();
        spawn->NPCPrefab = buffer.Read<size_t>();
        TraceLog(LOG_INFO, "Loaded NPCSpawnComponent for entity %zu", component->EntityID);
    }
}
