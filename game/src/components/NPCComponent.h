#pragma once

#include "raylib.h"

#include "EntitySystem.h"
#include "SpriteManager.h"

struct NPCComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(NPCComponent);

    float Size = 20;
    Color Tint = BLUE;
    double LastUpdateTime = 0;

    SpriteManager::SpriteInstance Sprite;

    void Update();
};
