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

GameState Task::GetBlocksState()
{
    if (BlocksState != GameState::AutoNextState)
        return BlocksState;

    return GetNextState(DependsOnState);
}