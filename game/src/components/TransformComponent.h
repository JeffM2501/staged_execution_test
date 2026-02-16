#pragma once

#include "raylib.h"
#include "raymath.h"

#include "EntitySystem.h"

struct TransformComponent : public EntitySystem::EntityComponent
{
    Vector2 Position = Vector2Zeros;
    Vector2 Velocity = Vector2Zeros;
    DECLARE_SIMPLE_COMPONENT(TransformComponent);
};

