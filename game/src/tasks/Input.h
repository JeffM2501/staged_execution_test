#pragma once

#include "TaskManager.h"

#include "raylib.h"

class InputTask : public Task
{
public:
    DECLARE_TASK(InputTask);
    InputTask();

protected:
    void Tick() override;
};
