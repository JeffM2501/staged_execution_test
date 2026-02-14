#include "Task.h"

void Task::Execute()
{
    Completed.store(false);
    RunOneFrame();
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
