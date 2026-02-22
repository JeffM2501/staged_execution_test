#pragma once
// ThreadedProcessor.h
// Simple, header-only templated worker that processes T objects on a single background thread.
// - pending deque: objects waiting to be processed
// - completed deque: processed objects
// - all queue operations protected by mutexes
// - background thread started once in ctor and waits on a condition_variable
// - processing callable is supplied at construction time
// - safe stop/join in destructor

#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <utility>
#include <cassert>

template<typename T>
class ThreadedProcessor
{
public:
    // Processor callable should accept T (or const T&) and return T (processed result).
    // We accept any callable and wrap it to a canonical `T(T)` form.
    template<typename F>
    explicit ThreadedProcessor(F&& processorFunc)
    {
        // wrap arbitrary callable so we can always call Processor(T)
        auto call = std::forward<F>(processorFunc);
        Processor = [call](T v) mutable -> T { return call(std::move(v)); };

        Running.store(true, std::memory_order_release);
        Worker = std::thread(&ThreadedProcessor::ThreadMain, this);
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

private:
    void ThreadMain()
    {
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
                // Optionally, you can rethrow or store an error indicator in completed queue.
                continue;
            }

            // Put into completed queue
            {
                std::lock_guard<std::mutex> lk(CompletedMutex);
                Completed.push_back(std::move(result));
            }
        }
    }

private:
    std::function<T(T)> Processor;
    mutable std::mutex PendingMutex;
    mutable std::mutex CompletedMutex;
    std::deque<T> Pending;
    std::deque<T> Completed;

    std::condition_variable Cv;
    std::thread Worker;
    std::atomic<bool> Running{ false };
};


#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <cassert>

namespace UnitTest
{

    // Wait until CompletedCount() reaches expected or until timeout_ms expires.
    // Returns true if expected reached, false on timeout.
    static bool waitForCompleted(ThreadedProcessor<int>& proc, size_t expected, int timeout_ms)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (proc.CompletedCount() >= expected) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return false;
    }

    int TestThreadedProcessor()
    {
        // Test 1: Single item processing
        {
            ThreadedProcessor<int> proc([](int v) { return v * 2; });
            proc.PushPending(21);

            bool ok = waitForCompleted(proc, 1, 1000);
            if (!ok)
            {
                std::cerr << "Test 1 timeout: item not processed\n";
                return 1;
            }

            int out = 0;
            bool popped = proc.PopCompleted(out);
            assert(popped && "Expected one completed item");
            assert(out == 42 && "Processed value must be doubled (21 -> 42)");
        }

        // Test 2: Multiple items processed in FIFO order (as pushed)
        {
            ThreadedProcessor<int> proc([](int v) {
                // small delay to simulate work & increase likelihood of concurrent timing differences
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return v + 100;
                });

            const int N = 8;
            for (int i = 0; i < N; ++i) proc.PushPending(i);

            bool ok = waitForCompleted(proc, N, 2000);
            if (!ok)
            {
                std::cerr << "Test 2 timeout: not all items processed\n";
                return 1;
            }

            std::vector<int> results;
            results.reserve(N);
            for (int i = 0; i < N; ++i)
            {
                int v;
                bool popped = proc.PopCompleted(v);
                assert(popped && "Expected available completed item");
                results.push_back(v);
            }

            // Validate all results are present and correct
            std::vector<int> expected;
            expected.reserve(N);
            for (int i = 0; i < N; ++i) expected.push_back(i + 100);

            // Completed order for this test is not strictly guaranteed across implementations,
            // but ThreadedProcessor processes items one-by-one; test both possibilities:
            if (results != expected)
            {
                // If order differs, ensure content matches (multithreading could reorder if processor is changed)
                std::sort(results.begin(), results.end());
                std::sort(expected.begin(), expected.end());
                assert(results == expected && "Processed results do not match expected values");
            }
        }

        // Test 3: Stop() and destructor join (no crash)
        {
            ThreadedProcessor<int> proc([](int v) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                return v * v;
                });

            proc.PushPending(5);
            proc.PushPending(6);

            bool ok = waitForCompleted(proc, 2, 2000);
            if (!ok)
            {
                std::cerr << "Test 3 timeout: not all items processed before Stop()\n";
                return 1;
            }

            // explicit Stop and join
            proc.Stop();

            // After Stop, completed items should still be retrievable
            int a, b;
            bool pa = proc.PopCompleted(a);
            bool pb = proc.PopCompleted(b);
            assert(pa && pb && "Expected two completed items after Stop()");
            // values 25 and 36 (squared)
            std::vector<int> got = { a, b };
            std::sort(got.begin(), got.end());
            assert(got[0] == 25 && got[1] == 36);
        }

        std::cout << "All ThreadedProcessor tests passed.\n";
        return 0;
    }
}