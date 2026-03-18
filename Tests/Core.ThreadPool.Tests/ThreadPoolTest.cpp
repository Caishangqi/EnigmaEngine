// Copyright EnigmaEngine. All Rights Reserved.

#include <gtest/gtest.h>
#include "Async/ThreadPool.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace Enigma;

// Test: Default constructor creates hardware_concurrency - 1 threads (min 1)
TEST(ThreadPoolTest, CreateDefault)
{
    FThreadPool pool;
    EXPECT_GE(pool.GetThreadCount(), 1u);
    EXPECT_TRUE(pool.IsRunning());
}

// Test: Explicit thread count is honored
TEST(ThreadPoolTest, CreateExplicit)
{
    FThreadPool pool(2);
    EXPECT_EQ(pool.GetThreadCount(), 2u);
    EXPECT_TRUE(pool.IsRunning());
}

// Test: Submit a single task and get result via future
TEST(ThreadPoolTest, SubmitAndWait)
{
    FThreadPool pool(2);
    auto future = pool.Submit([]() { return 42; });
    EXPECT_EQ(future.get(), 42);
}

// Test: Multiple tasks all execute and complete
TEST(ThreadPoolTest, SubmitMultiple)
{
    FThreadPool pool(4);
    constexpr int N = 100;
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    futures.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        futures.push_back(pool.Submit([&counter]() { counter.fetch_add(1); }));
    }

    for (auto& f : futures)
    {
        f.get();
    }

    EXPECT_EQ(counter.load(), N);
}

// Test: Shutdown drains all pending tasks before joining
TEST(ThreadPoolTest, ShutdownDrains)
{
    constexpr int N = 50;
    std::atomic<int> counter{0};

    {
        FThreadPool pool(2);
        for (int i = 0; i < N; ++i)
        {
            pool.Submit([&counter]()
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                counter.fetch_add(1);
            });
        }
        // Destructor calls Shutdown, which should drain
    }

    EXPECT_EQ(counter.load(), N);
}

// Test: Workers sleep when idle (basic smoke test -- pool doesn't burn CPU)
TEST(ThreadPoolTest, NoTasksSleep)
{
    FThreadPool pool(2);
    // Just verify pool is running and stable after brief idle period
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(pool.IsRunning());
    EXPECT_EQ(pool.GetThreadCount(), 2u);
}
