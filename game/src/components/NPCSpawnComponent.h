#pragma once

#include "EntitySystem.h"

struct NPCSpawnComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(NPCSpawnComponent);

    float MinInterval = 1.0f;
    float MaxInterval = 3.0f;

    float MinVelocity = 20.0f;
    float MaxVelocity = 100.0f;

    float MaxSpawnCount = 200.0f;

    size_t NPCPrefab = 0;

    double LastUpdateTime = 0;

    void Update();
};
