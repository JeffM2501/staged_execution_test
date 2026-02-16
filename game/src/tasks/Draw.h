#pragma once

#include "Task.h"

class DrawTask : public Task
{
public:
    DECLARE_TASK(DrawTask);
    DrawTask() :Task(FrameStage::Draw, true) {}

    void Tick() override;
};