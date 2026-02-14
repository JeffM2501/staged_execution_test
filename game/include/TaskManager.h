#pragma once

#include "Task.h"

#include <atomic>
#include <deque>
#include <functional>
#include <thread>

class ThreadInfo
{
private:
    std::atomic<bool> Running = false;
    std::atomic<bool> Abort = false;
public:
    size_t ThreadId = 0;
    std::thread Thread;
    std::deque<Task*> Tasks;

    std::function<void(Task*)> OnTaskComplete;

    std::function<void(size_t)> OnThreadIdle;

    void CheckTasks();

    void AbortTasks();

    bool IsIdle();

    void AddTask(Task* task);
};

#if defined(DEBUG)
struct GameStateStats
{
    size_t TaskCount = 0;
    double StartTime = 0;
    double EndTime = 0;

    double Durration = 0;
    double BlockedDurration = 0;

    double MaxDurration = 0;
    double MaxBlockedDurration = 0;
};
#endif

namespace TaskManager
{
    extern std::vector<std::unique_ptr<Task>> Tasks;

    void Init();
    void Cleanup();

    template<typename T, typename... Args>
    T* AddTask(Args&&... args)
    {
        auto task = std::make_unique<T>(std::forward<Args>(args)...);
        T* taskPtr = task.get();
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

    bool IsStateBlocked(GameState state);
    void AdvanceThreadIndex();
    void RunTasksForState(GameState state);
    bool IsIdle();
    void AddTask(std::unique_ptr<Task> task);
    void AbortAll();

#if defined(DEBUG)
    GameStateStats& GetStatsForState(GameState state);
#endif 
}
