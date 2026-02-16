#pragma once

#include "TaskManager.h"

class OverlayTask : public Task
{
public:
    DECLARE_TASK(OverlayTask);
    OverlayTask();

    void Tick() override;
};