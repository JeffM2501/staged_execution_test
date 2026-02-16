#pragma once

#include "Task.h"

class OverlayTask : public Task
{
public:
    DECLARE_TASK(OverlayTask);
    OverlayTask() : Task(FrameStage::Draw, true) { }
    
    void Tick() override;
};