#pragma once
#pragma once

#include "raylib.h"
#include "raymath.h"
#include "SpriteManager.h"

#include "EntitySystem.h"

struct BulletComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(BulletComponent);

    float Size = 4;
    Color Tint = YELLOW;
    double LastUpdateTime = 0;

    float Damage = 10;
    float Lifetime = 3.0f;

    float SpinDir = 1.0f;

    SpriteManager::SpriteInstance Sprite;

    void Update();

    void OnAwake() override;
};
