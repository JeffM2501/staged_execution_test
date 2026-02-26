#pragma once

#include "raylib.h"

#include "EntitySystem.h"

struct PlayerSpawnComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(PlayerSpawnComponent);

    float Size = 20;
    Color Tint = BLUE;
    double LastUpdateTime = 0;

    void Update();
};
