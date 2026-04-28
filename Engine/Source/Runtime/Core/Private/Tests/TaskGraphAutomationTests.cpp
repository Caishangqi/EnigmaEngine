// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Async/TaskGraph.h"
#include "Async/ThreadPool.h"

#include <atomic>
#include <mutex>
#include <vector>

namespace Enigma
{

#define ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(                                \
		TestClass,                                                           \
		PrettyName,                                                          \
		Core,                                                                \
		EAutomationTestType::Unit,                                           \
		EAutomationTestFlags::None)                                          \
	bool TestClass::RunTest(const FAutomationTestContext& Context)

ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST(
	FTaskGraphSingleTaskTest,
	"System.Core.TaskGraph.SingleTask")
{
	FThreadPool Pool(2);
	FTaskGraph Graph(Pool);

	bool bExecuted = false;
	Graph.CreateTask([&bExecuted]()
	{
		bExecuted = true;
	});
	Graph.Dispatch();
	Graph.WaitAll();

	TestTrue("Single task should execute", bExecuted);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST(
	FTaskGraphIndependentTasksTest,
	"System.Core.TaskGraph.IndependentTasks")
{
	FThreadPool Pool(4);
	FTaskGraph Graph(Pool);

	constexpr int TaskCount = 20;
	std::atomic<int> Counter{0};

	for (int Index = 0; Index < TaskCount; ++Index)
	{
		Graph.CreateTask([&Counter]()
		{
			Counter.fetch_add(1);
		});
	}

	Graph.Dispatch();
	Graph.WaitAll();

	TestEqual("All independent tasks should execute", Counter.load(), TaskCount);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST(
	FTaskGraphDependencyChainTest,
	"System.Core.TaskGraph.DependencyChain")
{
	FThreadPool Pool(2);
	FTaskGraph Graph(Pool);

	std::mutex Mutex;
	std::vector<int> Order;

	const FTaskHandle TaskA = Graph.CreateTask([&Mutex, &Order]()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Order.push_back(1);
	});
	const FTaskHandle TaskB = Graph.CreateTask([&Mutex, &Order]()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Order.push_back(2);
	}, {TaskA});
	Graph.CreateTask([&Mutex, &Order]()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Order.push_back(3);
	}, {TaskB});

	Graph.Dispatch();
	Graph.WaitAll();

	TestEqual("Dependency chain should produce three entries", Order.size(), 3u);
	if (Order.size() == 3u)
	{
		TestEqual("First dependency should execute first", Order[0], 1);
		TestEqual("Second dependency should execute second", Order[1], 2);
		TestEqual("Third dependency should execute third", Order[2], 3);
	}
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST(
	FTaskGraphDiamondDependencyTest,
	"System.Core.TaskGraph.DiamondDependency")
{
	FThreadPool Pool(4);
	FTaskGraph Graph(Pool);

	std::mutex Mutex;
	std::vector<int> Order;

	const FTaskHandle TaskA = Graph.CreateTask([&Mutex, &Order]()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Order.push_back(1);
	});
	const FTaskHandle TaskB = Graph.CreateTask([&Mutex, &Order]()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Order.push_back(2);
	}, {TaskA});
	const FTaskHandle TaskC = Graph.CreateTask([&Mutex, &Order]()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Order.push_back(3);
	}, {TaskA});
	Graph.CreateTask([&Mutex, &Order]()
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Order.push_back(4);
	}, {TaskB, TaskC});

	Graph.Dispatch();
	Graph.WaitAll();

	TestEqual("Diamond dependency should produce four entries", Order.size(), 4u);
	if (Order.size() == 4u)
	{
		TestEqual("Root dependency should execute first", Order[0], 1);
		TestEqual("Join dependency should execute last", Order[3], 4);
	}
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST(
	FTaskGraphSingleThreadFallbackTest,
	"System.Core.TaskGraph.SingleThreadFallback")
{
	FTaskGraph Graph;

	TestTrue("Task graph without thread pool should be single-threaded", Graph.IsSingleThreaded());

	std::vector<int> Order;
	const FTaskHandle TaskA = Graph.CreateTask([&Order]()
	{
		Order.push_back(1);
	});
	const FTaskHandle TaskB = Graph.CreateTask([&Order]()
	{
		Order.push_back(2);
	}, {TaskA});
	Graph.CreateTask([&Order]()
	{
		Order.push_back(3);
	}, {TaskB});

	Graph.Dispatch();
	Graph.WaitAll();

	TestEqual("Single-threaded graph should produce three entries", Order.size(), 3u);
	if (Order.size() == 3u)
	{
		TestEqual("First task should execute first", Order[0], 1);
		TestEqual("Second task should execute second", Order[1], 2);
		TestEqual("Third task should execute third", Order[2], 3);
	}
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST(
	FTaskGraphResetAndReuseTest,
	"System.Core.TaskGraph.ResetAndReuse")
{
	FThreadPool Pool(2);
	FTaskGraph Graph(Pool);

	std::atomic<int> Counter{0};
	Graph.CreateTask([&Counter]()
	{
		Counter.fetch_add(1);
	});
	Graph.Dispatch();
	Graph.WaitAll();
	TestEqual("First graph dispatch should execute", Counter.load(), 1);

	Graph.Reset();

	Graph.CreateTask([&Counter]()
	{
		Counter.fetch_add(10);
	});
	Graph.Dispatch();
	Graph.WaitAll();
	TestEqual("Reset graph should be reusable", Counter.load(), 11);
	return !Context.HasAnyFailures();
}

#undef ENIGMA_IMPLEMENT_TASK_GRAPH_AUTOMATION_TEST

} // namespace Enigma
