#pragma once

#include "raylib.h"
#include "raymath.h"

#include "EntitySystem.h"

struct PlayerComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(PlayerComponent);

    Vector2 Input = Vector2Zeros;

    bool ShootThisFrame = false;

    float Size = 10;
    float Health = 100;

    float PlayerSpeed = 100.0f;

    double LastShotTime = 0;

    float ReloadTime = 0.15f;

    size_t BulletPrefab = 0;

    void Update();
};