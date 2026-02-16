#pragma once

#include "EntitySystem.h"
#include "TaskManager.h"
#include "GameState.h"

template<class T>
void RegisterComponentWithUpdate(GameState state, bool threadUpdate)
{
    EntitySystem::RegisterComponent<T>();

    auto taskTick = [threadUpdate]()
        {
            EntitySystem::DoForEachComponent<T>([](T & component)
            {
                // TODO, check enabled?

                component.Update();
            },
            threadUpdate);
        };
    TaskManager::AddTaskOnState<LambdaTask>(state, T::GetComponentId(), taskTick);
}
#define SimpleComponentWithUpdate(T)