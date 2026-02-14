#pragma once

#include "GameState.h"
#include "CRC64.h"

#include <vector>
#include <atomic>
#include <memory>


#define DECLARE_TASK(TaskName) \
 size_t TaskId() override { return Hashes::CRC64Str(#TaskName); } \
 static size_t GetTaskId() { return Hashes::CRC64Str(#TaskName); }

class Task
{
protected:
    std::atomic<bool> Completed = false;

    virtual void RunOneFrame() = 0;
public:
    virtual size_t TaskId() = 0;

    void Execute();

    bool IsComplete();

    template<typename T, typename... Args>
    T* AddDependency(Args&&... args)
    {
        auto task = std::make_unique<T>(std::forward<Args>(args)...);
        T* taskPtr = task.get();
        Dependencies.emplace_back(std::move(task));
        return taskPtr;
    }

    template<typename T>
    T* GetTask()
    {
        if (TaskId() == T::GetTaskId())
            return static_cast<T*>(this);

        for (auto& dependency : Dependencies)
        {
            T* found = dependency->GetTask<T>();
            if (found)
                return found;
        }
        return nullptr;
    }

    GameState DependsOnState = GameState::FrameHead;
    GameState BlocksState = GameState::FrameTail;

    bool RunInMainThread = false;

    std::vector<std::unique_ptr<Task>> Dependencies;
};


