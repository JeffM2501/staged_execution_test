#pragma once

#include "TaskManager.h"

#include "raylib.h"

class GUITask : public Task
{
private:
    Texture     Logo;

public:
    DECLARE_TASK(GUITask);
    GUITask();
    ~GUITask();

    void Tick() override;
};