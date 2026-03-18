// Copyright EnigmaEngine. All Rights Reserved.

#include <gtest/gtest.h>
#include "Async/TaskGraph.h"
#include "Async/ThreadPool.h"

#include <atomic>
#include <mutex>
#include <vector>

using namespace Enigma;

// Test: Single task dispatches and completes
TEST(TaskGraphTest, SingleTask)
{
    FThreadPool pool(2);
    FTaskGraph graph(pool);

    bool executed = false;
    graph.CreateTask([&executed]() { executed = true; });
    graph.Dispatch();
    graph.WaitAll();

    EXPECT_TRUE(executed);
}

// Test: Multiple independent tasks all complete
TEST(TaskGraphTest, IndependentTasks)
{
    FThreadPool pool(4);
    FTaskGraph graph(pool);

    constexpr int N = 20;
    std::atomic<int> counter{0};

    for (int i = 0; i < N; ++i)
    {
        graph.CreateTask([&counter]() { counter.fetch_add(1); });
    }

    graph.Dispatch();
    graph.WaitAll();

    EXPECT_EQ(counter.load(), N);
}

// Test: Dependency chain A->B->C executes in order
TEST(TaskGraphTest, DependencyChain)
{
    FThreadPool pool(2);
    FTaskGraph graph(pool);

    std::mutex mtx;
    std::vector<int> order;

    auto hA = graph.CreateTask([&]() { std::lock_guard<std::mutex> lk(mtx); order.push_back(1); });
    auto hB = graph.CreateTask([&]() { std::lock_guard<std::mutex> lk(mtx); order.push_back(2); }, {hA});
    auto hC = graph.CreateTask([&]() { std::lock_guard<std::mutex> lk(mtx); order.push_back(3); }, {hB});

    graph.Dispatch();
    graph.WaitAll();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// Test: Diamond dependency A->{B,C}->D executes D last
TEST(TaskGraphTest, DiamondDependency)
{
    FThreadPool pool(4);
    FTaskGraph graph(pool);

    std::mutex mtx;
    std::vector<int> order;

    auto hA = graph.CreateTask([&]() { std::lock_guard<std::mutex> lk(mtx); order.push_back(1); });
    auto hB = graph.CreateTask([&]() { std::lock_guard<std::mutex> lk(mtx); order.push_back(2); }, {hA});
    auto hC = graph.CreateTask([&]() { std::lock_guard<std::mutex> lk(mtx); order.push_back(3); }, {hA});
    auto hD = graph.CreateTask([&]() { std::lock_guard<std::mutex> lk(mtx); order.push_back(4); }, {hB, hC});

    graph.Dispatch();
    graph.WaitAll();

    ASSERT_EQ(order.size(), 4u);
    EXPECT_EQ(order[0], 1);  // A first
    EXPECT_EQ(order[3], 4);  // D last
}

// Test: Single-threaded fallback executes all tasks on calling thread
TEST(TaskGraphTest, SingleThreadFallback)
{
    FTaskGraph graph; // no thread pool = single-threaded

    EXPECT_TRUE(graph.IsSingleThreaded());

    std::vector<int> order;
    auto hA = graph.CreateTask([&]() { order.push_back(1); });
    auto hB = graph.CreateTask([&]() { order.push_back(2); }, {hA});
    auto hC = graph.CreateTask([&]() { order.push_back(3); }, {hB});

    graph.Dispatch();
    graph.WaitAll();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// Test: Reset allows graph reuse across frames
TEST(TaskGraphTest, ResetAndReuse)
{
    FThreadPool pool(2);
    FTaskGraph graph(pool);

    // Frame 1
    std::atomic<int> counter{0};
    graph.CreateTask([&counter]() { counter.fetch_add(1); });
    graph.Dispatch();
    graph.WaitAll();
    EXPECT_EQ(counter.load(), 1);

    // Reset and reuse
    graph.Reset();

    graph.CreateTask([&counter]() { counter.fetch_add(10); });
    graph.Dispatch();
    graph.WaitAll();
    EXPECT_EQ(counter.load(), 11);
}
