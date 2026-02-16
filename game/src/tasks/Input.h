#pragma once

#include "Task.h"

#include "raylib.h"

// TODO, replace with an action system that installs it's own task
class InputTask : public Task
{
public:
    DECLARE_TASK(InputTask);
    InputTask() : Task(FrameStage::PreUpdate, true){}

protected:
    void Tick() override;
};
