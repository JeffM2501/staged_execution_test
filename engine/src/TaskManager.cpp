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

        IsProcessing.store(true);
        Task* task = nullptr;
        {
            std::lock_guard<std::mutex> taskLock(TaskLock);
      
            task = Tasks.front();
            Tasks.pop_front();
        }
       
        task->Execute();
       
        if (OnTaskComplete)
            OnTaskComplete(task);

        IsProcessing.store(false);
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
    std::lock_guard<std::mutex> lock(TaskLock);
    return Tasks.empty() && !IsProcessing.load();
}

void ThreadInfo::AddTask(Task* task)
{
    {
        std::lock_guard<std::mutex> lock(TaskLock);
        Tasks.push_back(task);
    }
    Trigger.notify_one();
}

namespace TaskManager
{
    std::vector<std::unique_ptr<Task>> Tasks;
    std::vector<std::unique_ptr<ThreadInfo>> Threads;

    // caches by stage
    std::unordered_map<FrameStage, std::vector<Task*>> TasksPerStartStage;
    std::unordered_map<FrameStage, std::vector<Task*>> TasksBlockingStages;

#if defined(DEBUG)
    std::unordered_map<FrameStage, FrameStageStats> StageStats;
   
    FrameStageStats& GetStatsForStage(FrameStage stage)
    {
        if (!StageStats.contains(stage))
            StageStats[stage] = FrameStageStats();

        return StageStats[stage];
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

    void Shutdown()
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

        for (FrameStage stage = FrameStage::FrameHead; stage <= FrameStage::FrameTail; ++stage)
        {
#if defined(DEBUG)
            auto& stats = GetStatsForStage(stage);
            stats.TickedThisFrame = false;
#endif
            if (stage == FrameStage::FixedUpdate)
            {
                while (Accumulator >= FixedUpdateTime)
                {
                    RunTasksForStage(FrameStage::FixedUpdate);
                    Accumulator -= FixedUpdateTime;
                }
            }
            else
            {
                RunTasksForStage(stage);
                if (stage == FrameStage::Present)
                    EndDrawing();
            }
        }
    }

    bool IsStageBlocked(FrameStage stage)
    {
        if (!TasksBlockingStages.contains(stage))
            return false;

        for (auto& task : TasksBlockingStages[stage])
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

    void RunTasksForStage(FrameStage stage)
    {
#if defined(DEBUG)
        auto& stats = GetStatsForStage(stage);
        stats.TaskCount = 0;
        stats.BlockedDurration = 0;
        stats.Durration = 0;
        stats.StartTime = GetTime();
        stats.TickedThisFrame = true;
#endif
        bool wasBlocked = false;
        while (IsStageBlocked(stage))
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

        if (TasksPerStartStage.contains(stage))
        {
            for (auto task : TasksPerStartStage[stage])
            {
                // save off the blocking stage
                TasksBlockingStages[task->GetBlocksStage()].push_back(task);

                if (task->RunInMainThread)
                    continue;

                Threads[GetAvailableThread()]->AddTask(task);
#if defined(DEBUG)
                stats.TaskCount++;
#endif
            }

            for (auto task : TasksPerStartStage[stage])
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

    void CacheStageTask(Task* task)
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