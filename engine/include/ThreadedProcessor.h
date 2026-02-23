#pragma once
// ThreadedProcessor.h
// Simple, header-only templated worker that processes T objects on a single background thread.
// - pending deque: objects waiting to be processed
// - completed deque: processed objects
// - all queue operations protected by mutexes
// - background thread started once in ctor and waits on a condition_variable
// - processing callable is supplied at construction time (or later via SetProcessorAndStart)
// - safe stop/join in destructor
// - optional thread lifecycle callbacks: OnThreadStart, OnThreadStop

#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <utility>
#include <cassert>

#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

template<typename T>
class ThreadedProcessor
{
public:
    // Default constructor: does not start the worker thread.
    ThreadedProcessor() = default;

    // Constructor that sets processor and starts the thread immediately.
    // Optional lifecycle callbacks can be provided.
    template<typename F>
    explicit ThreadedProcessor(F&& processorFunc,
        const std::function<void()>& onStart = nullptr,
        const std::function<void()>& onEnd = nullptr)
    {
        // Delegate to SetProcessorAndStart to avoid duplication
        SetProcessorAndStart(std::forward<F>(processorFunc), onStart, onEnd);
    }

    // Not copyable
    ThreadedProcessor(const ThreadedProcessor&) = delete;
    ThreadedProcessor& operator=(const ThreadedProcessor&) = delete;

    // Movable (thread ownership is not moved; disallow move for simplicity)
    ThreadedProcessor(ThreadedProcessor&&) = delete;
    ThreadedProcessor& operator=(ThreadedProcessor&&) = delete;

    ~ThreadedProcessor()
    {
        Stop();
    }

    // Push into pending queue (copy)
    void PushPending(const T& item)
    {
        {
            std::lock_guard<std::mutex> lk(PendingMutex);
            Pending.push_back(item);
        }
        Cv.notify_one();
    }

    // Push into pending queue (move)
    void PushPending(T&& item)
    {
        {
            std::lock_guard<std::mutex> lk(PendingMutex);
            Pending.push_back(std::move(item));
        }
        Cv.notify_one();
    }

    // Non-blocking pop from pending queue (user-side). Returns true if an item was returned.
    bool PopPending(T& out)
    {
        std::lock_guard<std::mutex> lk(PendingMutex);
        if (Pending.empty()) return false;
        out = std::move(Pending.front());
        Pending.pop_front();
        return true;
    }

    // Push directly into completed queue (copy)
    void PushCompleted(const T& item)
    {
        std::lock_guard<std::mutex> lk(CompletedMutex);
        Completed.push_back(item);
    }

    // Push directly into completed queue (move)
    void PushCompleted(T&& item)
    {
        std::lock_guard<std::mutex> lk(CompletedMutex);
        Completed.push_back(std::move(item));
    }

    // Non-blocking pop from completed queue (user-side). Returns true if an item was returned.
    bool PopCompleted(T& out)
    {
        std::lock_guard<std::mutex> lk(CompletedMutex);
        if (Completed.empty()) return false;
        out = std::move(Completed.front());
        Completed.pop_front();
        return true;
    }

    // Returns number of pending items (approximate; may change immediately after call)
    size_t PendingCount() const
    {
        std::lock_guard<std::mutex> lk(PendingMutex);
        return Pending.size();
    }

    // Returns number of completed items (approximate)
    size_t CompletedCount() const
    {
        std::lock_guard<std::mutex> lk(CompletedMutex);
        return Completed.size();
    }

    // Request a graceful stop and join the worker thread.
    void Stop()
    {
        bool expected = true;
        if (Running.compare_exchange_strong(expected, false, std::memory_order_acq_rel))
        {
            Cv.notify_all();
            if (Worker.joinable()) Worker.join();
        }
    }

    // Set optional callbacks for thread lifecycle.
    // Thread-safety: the setter grabs CallbackMutex to assign the callback.
    void SetOnThreadStart(const std::function<void()>& cb)
    {
        std::lock_guard<std::mutex> lk(CallbackMutex);
        OnThreadStart = cb;
    }
    void SetOnThreadStart(std::function<void()>&& cb)
    {
        std::lock_guard<std::mutex> lk(CallbackMutex);
        OnThreadStart = std::move(cb);
    }

    void SetOnThreadStop(const std::function<void()>& cb)
    {
        std::lock_guard<std::mutex> lk(CallbackMutex);
        OnThreadStop = cb;
    }
    void SetOnThreadStop(std::function<void()>&& cb)
    {
        std::lock_guard<std::mutex> lk(CallbackMutex);
        OnThreadStop = std::move(cb);
    }

    // Set processor function and start the worker thread.
    // Returns true if the thread was started, false if already running or already started before.
    template<typename F>
    bool SetProcessorAndStart(F&& processorFunc,
        const std::function<void()>& onStart = nullptr,
        const std::function<void()>& onEnd = nullptr)
    {
        std::lock_guard<std::mutex> lk(StartMutex);

        // If already running, do not start again
        if (Running.load(std::memory_order_acquire)) return false;
        // If worker is joinable it means it was started previously and not joinable? treat as cannot restart.
        if (Worker.joinable()) return false;

        // wrap arbitrary callable so we can always call Processor(T)
        auto call = std::forward<F>(processorFunc);
        Processor = [call](T v) mutable -> T { return call(std::move(v)); };

        if (onStart) SetOnThreadStart(onStart);
        if (onEnd) SetOnThreadStop(onEnd);

        Running.store(true, std::memory_order_release);
        Worker = std::thread(&ThreadedProcessor::ThreadMain, this);
        return true;
    }

private:
    void ThreadMain()
    {
        // Invoke start callback if set
        {
            std::function<void()> cb;
            {
                std::lock_guard<std::mutex> lk(CallbackMutex);
                cb = OnThreadStart;
            }
            if (cb) cb();
        }

        while (true)
        {
            T item;
            {
                std::unique_lock<std::mutex> lk(PendingMutex);
                Cv.wait(lk, [this] { return !Pending.empty() || !Running.load(std::memory_order_acquire); });

                if (Pending.empty())
                {
                    // If no pending items and not running -> shutdown.
                    if (!Running.load(std::memory_order_acquire)) break;
                    continue; // spurious wake or running changed
                }

                // get one item
                item = std::move(Pending.front());
                Pending.pop_front();
            }

            // Process outside of the pending lock
            T result;
            try
            {
                result = Processor(std::move(item));
            }
            catch (...)
            {
                // Swallow exceptions to keep worker alive. In case of error, user can push error sentinel into completed.
                continue;
            }

            // Put into completed queue
            {
                std::lock_guard<std::mutex> lk(CompletedMutex);
                Completed.push_back(std::move(result));
            }
        }

        // Invoke stop callback if set
        {
            std::function<void()> cb;
            {
                std::lock_guard<std::mutex> lk(CallbackMutex);
                cb = OnThreadStop;
            }
            if (cb) cb();
        }
    }

private:
    // Processing callable
    std::function<T(T)> Processor;

    // Queues and locks
    mutable std::mutex PendingMutex;
    mutable std::mutex CompletedMutex;
    std::deque<T> Pending;
    std::deque<T> Completed;

    // Thread and synchronization
    std::condition_variable Cv;
    std::thread Worker;
    std::atomic<bool> Running{ false };

    // Optional lifecycle callbacks
    std::mutex CallbackMutex;
    std::function<void()> OnThreadStart;
    std::function<void()> OnThreadStop;

    // Guard for starting the worker once
    std::mutex StartMutex;
};
