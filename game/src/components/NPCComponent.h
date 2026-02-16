#pragma once

#include "raylib.h"
#include "raymath.h"

#include "EntitySystem.h"

struct NPCComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(NPCComponent);

    float Size = 20;
    Color Tint = BLUE;
    double LastUpdateTime = 0;

    void Update();
};
