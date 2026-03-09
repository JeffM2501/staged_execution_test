#pragma once

#include "raylib.h"
#include "raymath.h"

#include "EntitySystem.h"
#include "SpriteManager.h"
#include "TransformComponent.h"

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

    float ShotSpread = 50.0f;
    float ShotSpeedMultiplyer = 3;
    float ShotSpeedVariance = 1.0f;

    SpriteManager::SpriteInstance Sprite;

    void Update();

    void OnAwake() override;

    bool OnDataRead(BufferReader& buffer) override;
};