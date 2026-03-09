#pragma once

#include "raylib.h"
#include "raymath.h"

#include "EntitySystem.h"

struct TransformComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(TransformComponent);

    bool OnDataRead(BufferReader& buffer) override
    {
        Position.x = buffer.Read<float>();
        Position.y = buffer.Read<float>();
        Velocity.x = buffer.Read<float>();
        Velocity.y = buffer.Read<float>();

        TraceLog(LOG_INFO, "Loaded Transform for entity %zu", EntityID);

        return true;
    }

    Vector2 Position = Vector2Zeros;
    Vector2 Velocity = Vector2Zeros;
};

