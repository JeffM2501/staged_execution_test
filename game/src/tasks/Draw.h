#pragma once

#include "TaskManager.h"

class DrawTask : public Task
{
public:
    DECLARE_TASK(DrawTask);
    DrawTask();

    void Tick() override;
};