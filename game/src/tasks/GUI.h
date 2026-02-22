#pragma once

#include "Task.h"

#include "raylib.h"
#include "TextureManager.h"

class GUITask : public Task
{
private:
    TextureManager::TextureReference     Logo;

public:
    DECLARE_TASK(GUITask);
    GUITask();
    ~GUITask();

    void Tick() override;
};