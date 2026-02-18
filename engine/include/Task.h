#pragma once

#include "FrameStage.h"
#include "CRC64.h"

#include <vector>
#include <atomic>
#include <memory>
#include <functional>

#define DECLARE_TASK(TaskName) \
 size_t TaskId() override { return Hashes::CRC64Str(#TaskName); } \
 static size_t GetTaskId() { return Hashes::CRC64Str(#TaskName); }

class Task
{
protected:
    std::atomic<bool> Completed = false;

    virtual void Tick() = 0;

    FrameStage BlocksStage = FrameStage::AutoNextState;
public:
    virtual size_t TaskId() = 0;

    Task() = default;
    Task(FrameStage startStage, bool mainTherad) : StartingStage(startStage), RunInMainThread(mainTherad) {}

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

    FrameStage StartingStage = FrameStage::FrameHead;
   
    FrameStage GetBlocksStage();

    bool RunInMainThread = false;

    std::vector<std::unique_ptr<Task>> Dependencies;
    std::atomic<bool> TickedThisFrame = false;
};

class LambdaTask : public Task
{
private:
    size_t TaskHash = 0;
    std::function<void()> TickFunction;

protected:
    void Tick()
    {
        if (TickFunction)
            TickFunction();
    }
public:
    LambdaTask(size_t taskHash, std::function<void()> tick, bool useMainThread = false) 
        : TaskHash(taskHash)
        , TickFunction(tick) 
    { 
        RunInMainThread = useMainThread;
        StartingStage = FrameStage::None;
    }

    size_t TaskId() override { return TaskHash; }
};


