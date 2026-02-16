#include "Task.h"

void Task::Execute()
{
    TickedThisFrame.store(true);
    Completed.store(false);
    Tick();
    for (auto& dependency : Dependencies)
    {
        dependency->Execute();
    }
    Completed.store(true);
}

bool Task::IsComplete()
{
    return Completed.load();
}

FrameStage Task::GetBlocksStage()
{
    if (BlocksStage != FrameStage::AutoNextState)
        return BlocksStage;

    return GetNextStage(StartingStage);
}