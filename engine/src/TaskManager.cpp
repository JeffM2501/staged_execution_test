#include "TaskManager.h"
#include "TimeUtils.h"

#include "raylib.h"

#include <unordered_map>

void ThreadInfo::CheckTasks()
{
    if (Running.load())
        return;
    if (Tasks.empty())
        return;

    if (Thread.joinable())
        Thread.join();

    Thread = std::thread([this]()
        {
            Running.store(true);
            while (!Tasks.empty())
            {
                if (Abort.load())
                {
                    Tasks.clear();
                    break;
                }
                Task* task = Tasks.front();
                Tasks.pop_front();
                task->Execute();
                if (OnTaskComplete)
                    OnTaskComplete(task);
            }
            Running.store(false);

            if (OnThreadIdle)
                OnThreadIdle(ThreadId);
        });
}

void ThreadInfo::AbortTasks()
{
    Abort.store(true);
    if (Thread.joinable())
        Thread.join();
}

bool ThreadInfo::IsIdle()
{
    for (auto& task : Tasks)
    {
        if (!task->IsComplete())
            return false;
    }
    return Running.load();
}

void ThreadInfo::AddTask(Task* task)
{
    Tasks.push_back(task);
    CheckTasks();
}

namespace TaskManager
{
    std::vector<std::unique_ptr<Task>> Tasks;
    std::vector<std::unique_ptr<ThreadInfo>> Threads;

#if defined(DEBUG)
    std::unordered_map<FrameStage, GameStateStats> StateStats;
   
    GameStateStats& GetStatsForState(FrameStage state)
    {
        if (!StateStats.contains(state))
            StateStats[state] = GameStateStats();

        return StateStats[state];
    }
#endif 

    size_t NextThreadIndex = 0;

    float FixedUpdateTime = 1.0f / FixedFPS;
    float Accumulator = FixedUpdateTime;

    void Init()
    {
        for (size_t i = 0; i < std::thread::hardware_concurrency(); i++)
        {
            auto threadInfo = std::make_unique<ThreadInfo>();
            threadInfo->ThreadId = i;
            threadInfo->OnThreadIdle = [](size_t threadId)
                {
                    // handle idle thread
                };
            Threads.push_back(std::move(threadInfo));
        }
    }

    void Cleanup()
    {
        for (auto& thread : Threads)
        {
            thread->AbortTasks();
        }
        Threads.clear();
        Tasks.clear();
    }

    void TickFrame()
    {
        for (auto& task : Tasks)
            task->TickedThisFrame.store(false);

        Accumulator += GetDeltaTime();

        for (FrameStage state = FrameStage::FrameHead; state <= FrameStage::FrameTail; ++state)
        {
#if defined(DEBUG)
            auto& stats = GetStatsForState(state);
            stats.TickedThisFrame = false;
#endif
            if (state == FrameStage::FixedUpdate)
            {
                while (Accumulator >= FixedUpdateTime)
                {
                    RunTasksForState(FrameStage::FixedUpdate);
                    Accumulator -= FixedUpdateTime;
                }
            }
            else
            {
                RunTasksForState(state);
                if (state == FrameStage::Present)
                    EndDrawing();
            }
        }
    }

    bool IsStateBlocked(FrameStage state)
    {
        for (auto& task : Tasks)
        {
            if (task->GetBlocksStage() == state && !task->IsComplete())
            {
                return true;
            }
        }
        return false;
    }

    void AdvanceThreadIndex()
    {
        NextThreadIndex++;
        if (NextThreadIndex >= Threads.size())
            NextThreadIndex = 0;
    }

    void RunTasksForState(FrameStage state)
    {
#if defined(DEBUG)
        auto& stats = GetStatsForState(state);
        stats.TaskCount = 0;
        stats.BlockedDurration = 0;
        stats.Durration = 0;
        stats.StartTime = GetTime();
        stats.TickedThisFrame = true;
#endif
        bool wasBlocked = false;
        while (IsStateBlocked(state))
        {
            wasBlocked = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (wasBlocked)
        {
#if defined(DEBUG)
            stats.BlockedDurration = GetTime() - stats.StartTime;

            if (stats.BlockedDurration > stats.MaxBlockedDurration)
                stats.MaxBlockedDurration = stats.BlockedDurration;
#endif
        }

        for (auto& task : Tasks)
        {
            if (task->StartingStage == state && !task->RunInMainThread)
            {
                Threads[NextThreadIndex]->AddTask(task.get());
                AdvanceThreadIndex();

#if defined(DEBUG)
                stats.TaskCount++;
#endif
            }
        }

        for (auto& task : Tasks)
        {
            if (task->StartingStage == state && task->RunInMainThread)
            {
                task->Execute();
#if defined(DEBUG)
                stats.TaskCount++;
#endif
            }
        }

#if defined(DEBUG)
        stats.EndTime = GetTime();
        stats.Durration = stats.EndTime - stats.StartTime;
        if (stats.Durration > stats.MaxDurration)
            stats.MaxDurration = stats.Durration;
#endif
    }

    bool IsIdle()
    {
        for (auto& thread : Threads)
        {
            if (!thread->IsIdle())
                return false;
        }

        return true;
    }

    void AddTask(std::unique_ptr<Task> task)
    {
        Tasks.push_back(std::move(task));
    }

    void AbortAll()
    {
        for (auto& thread : Threads)
        {
            thread->AbortTasks();
        }
    }
}