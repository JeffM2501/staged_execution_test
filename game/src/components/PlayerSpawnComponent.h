#pragma once

#include "raylib.h"

#include "EntitySystem.h"

struct PlayerSpawnComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(PlayerSpawnComponent);

    double LastUpdateTime = 0;

    size_t PlayerPrefab = 0;

    void OnAwake() override;
};
