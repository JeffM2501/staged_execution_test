#pragma once

#include "raylib.h"
#include "raymath.h"

#include "EntitySystem.h"

struct TransformComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(TransformComponent);

    Vector2 Position = Vector2Zeros;
    Vector2 Velocity = Vector2Zeros;
};

