#include "TaskManager.h"
#include "TimeUtils.h"

#include "raylib.h"

#include <unordered_map>

ThreadInfo::ThreadInfo() : Running(true)
{
    Thread = std::thread([this]()
        {
            Run();
        });
}

ThreadInfo::~ThreadInfo()
{
    AbortTasks();
}

void ThreadInfo::Run()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(Lock);
        Trigger.wait(lock, [this]() { return !Running || !Tasks.empty(); });
        if (!Running && Tasks.empty())
            break;

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
    if (OnThreadAbort)
        OnThreadAbort(ThreadId);
}

void ThreadInfo::AbortTasks()
{
    {
        std::lock_guard<std::mutex> lock(Lock);
        Running.store(false);
        Abort.store(true);
    }
    Trigger.notify_one();

    if (Thread.joinable())
        Thread.join();
}

bool ThreadInfo::IsIdle()
{
    std::lock_guard<std::mutex> lock(Lock);
    for (auto& task : Tasks)
    {
        if (!task->IsComplete())
            return false;
    }
    return Running.load();
}

void ThreadInfo::AddTask(Task* task)
{
    {
        std::lock_guard<std::mutex> lock(Lock);
        Tasks.push_back(task);
    }
    Trigger.notify_one();
}

namespace TaskManager
{
    std::vector<std::unique_ptr<Task>> Tasks;
    std::vector<std::unique_ptr<ThreadInfo>> Threads;

    std::unordered_map<FrameStage, std::vector<Task*>> TasksPerStartStage;
    std::unordered_map<FrameStage, std::vector<Task*>> TasksBlockingStages;

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

        for (auto& [stage, blockerList] : TasksBlockingStages)
            blockerList.clear();

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
        if (!TasksBlockingStages.contains(state))
            return false;

        for (auto& task : TasksBlockingStages[state])
        {
            if (!task->IsComplete())
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

    size_t GetAvailableThread()
    {
        for (size_t i = 0; i < Threads.size(); i++)
        {
            if (Threads[i]->IsIdle())
                return i;
        }

        // just pick one
        AdvanceThreadIndex();
        return NextThreadIndex;
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

        if (TasksPerStartStage.contains(state))
        {
            for (auto task : TasksPerStartStage[state])
            {
                // save off the blocking state
                TasksBlockingStages[task->GetBlocksStage()].push_back(task);

                if (task->RunInMainThread)
                    continue;

                Threads[GetAvailableThread()]->AddTask(task);
#if defined(DEBUG)
                stats.TaskCount++;
#endif
            }

            for (auto task : TasksPerStartStage[state])
            {
                if (!task->RunInMainThread)
                    continue;

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

    void RunOneShotTask(Task* task)
    {
        if (!task)
            return;

        if (task->RunInMainThread)
        {
            task->Execute();
        }
        else
        {
            Threads[GetAvailableThread()]->AddTask(task);
        }
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

    void CacheStateTask(Task* task)
    {
        if (!TasksPerStartStage.contains(task->StartingStage))
        {
            TasksPerStartStage.try_emplace(task->StartingStage);
        }
        TasksPerStartStage[task->StartingStage].push_back(task);

        if (!TasksBlockingStages.contains(task->GetBlocksStage()))
            TasksBlockingStages.try_emplace(task->GetBlocksStage());
    }

    void AbortAll()
    {
        for (auto& thread : Threads)
        {
            thread->AbortTasks();
        }
    }
}