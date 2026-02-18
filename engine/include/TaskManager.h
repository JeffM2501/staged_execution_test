#pragma once

#include "Task.h"

#include <atomic>
#include <deque>
#include <functional>
#include <thread>
#include <algorithm>
#include <mutex>
#include <condition_variable>

class ThreadInfo
{
private:
    std::atomic<bool> Running = false;
    std::atomic<bool> Abort = false;

    void Run();

public:
    size_t ThreadId = 0;
    std::thread Thread;
    std::deque<Task*> Tasks;
    std::condition_variable Trigger;
    std::mutex  Lock;
    std::mutex  TaskLock;

    std::atomic<bool> IsProcessing = false;

    std::function<void(Task*)> OnTaskComplete;

    std::function<void(size_t)> OnThreadAbort;

    ThreadInfo();
    ~ThreadInfo();

    void AbortTasks();

    bool IsIdle();

    void AddTask(Task* task);
};

#if defined(DEBUG)
struct FrameStageStats
{
    size_t TaskCount = 0;
    double StartTime = 0;
    double EndTime = 0;

    double Durration = 0;
    double BlockedDurration = 0;

    double MaxDurration = 0;
    double MaxBlockedDurration = 0;

    bool TickedThisFrame = false;
};
#endif

namespace TaskManager
{
    extern std::vector<std::unique_ptr<Task>> Tasks;

    static constexpr float FixedFPS = 50.0f;

    void Init();
    void Cleanup();

    void TickFrame();

    void CacheStageTask(Task* task);

    template<typename T, typename... Args>
    T* AddTask(Args&&... args)
    {
        auto task = std::make_unique<T>(std::forward<Args>(args)...);
        T* taskPtr = task.get();
        CacheStageTask(taskPtr);
        Tasks.push_back(std::move(task));
        return taskPtr;
    }

    template<typename T, typename... Args>
    T* AddTaskOnState(FrameStage stage, Args&&... args)
    {
        auto task = std::make_unique<T>(std::forward<Args>(args)...);
        task->StartingStage = stage;
        T* taskPtr = task.get();
        CacheStageTask(taskPtr);
        Tasks.push_back(std::move(task));
        return taskPtr;
    }

    template<typename T>
    void RemoveTask()
    {
        size_t taskId = T::GetTaskId();
        Tasks.erase(std::remove_if(Tasks.begin(), Tasks.end(),
            [taskId](const std::unique_ptr<Task>& task)
            {
                return task->TaskId() == taskId;
            }), Tasks.end());
    }

    template<typename T>
    T* GetTask()
    {
        for (auto& task : Tasks)
        {
            T* found = task->GetTask<T>();
            if (found)
                return found;
        }
        return nullptr;
    }

    bool IsStageBlocked(FrameStage stage);
    void AdvanceThreadIndex();
    void RunTasksForStage(FrameStage state);

    void RunOneShotTask(Task* task);

    bool IsIdle();

    void AbortAll();

    inline float GetFixedDeltaTime() { return 1.0f / FixedFPS; }

#if defined(DEBUG)
    FrameStageStats& GetStatsForStage(FrameStage state);
#endif 
}
