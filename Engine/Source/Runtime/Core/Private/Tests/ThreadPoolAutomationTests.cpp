// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Async/ThreadPool.h"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

namespace Enigma
{

#define ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST(TestClass, PrettyName) \
	ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(                                \
		TestClass,                                                           \
		PrettyName,                                                          \
		Core,                                                                \
		EAutomationTestType::Unit,                                           \
		EAutomationTestFlags::None)                                          \
	bool TestClass::RunTest(const FAutomationTestContext& Context)

ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST(
	FThreadPoolCreateDefaultTest,
	"System.Core.ThreadPool.CreateDefault")
{
	FThreadPool Pool;
	TestTrue("Default thread pool should create at least one worker", Pool.GetThreadCount() >= 1u);
	TestTrue("Default thread pool should be running", Pool.IsRunning());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST(
	FThreadPoolCreateExplicitTest,
	"System.Core.ThreadPool.CreateExplicit")
{
	FThreadPool Pool(2);
	TestEqual("Explicit thread count should be honored", Pool.GetThreadCount(), 2u);
	TestTrue("Explicit thread pool should be running", Pool.IsRunning());
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST(
	FThreadPoolSubmitAndWaitTest,
	"System.Core.ThreadPool.SubmitAndWait")
{
	FThreadPool Pool(2);
	std::future<int> Future = Pool.Submit([]() { return 42; });
	TestEqual("Submitted task should return result through future", Future.get(), 42);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST(
	FThreadPoolSubmitMultipleTest,
	"System.Core.ThreadPool.SubmitMultiple")
{
	FThreadPool Pool(4);
	constexpr int TaskCount = 100;
	std::atomic<int> Counter{0};
	std::vector<std::future<void>> Futures;
	Futures.reserve(TaskCount);

	for (int Index = 0; Index < TaskCount; ++Index)
	{
		Futures.push_back(Pool.Submit([&Counter]()
		{
			Counter.fetch_add(1);
		}));
	}

	for (auto& Future : Futures)
	{
		Future.get();
	}

	TestEqual("All submitted tasks should execute", Counter.load(), TaskCount);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST(
	FThreadPoolShutdownDrainsTest,
	"System.Core.ThreadPool.ShutdownDrains")
{
	constexpr int TaskCount = 50;
	std::atomic<int> Counter{0};

	{
		FThreadPool Pool(2);
		for (int Index = 0; Index < TaskCount; ++Index)
		{
			Pool.Submit([&Counter]()
			{
				std::this_thread::sleep_for(std::chrono::microseconds(100));
				Counter.fetch_add(1);
			});
		}
	}

	TestEqual("Thread pool shutdown should drain pending tasks", Counter.load(), TaskCount);
	return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST(
	FThreadPoolNoTasksSleepTest,
	"System.Core.ThreadPool.NoTasksSleep")
{
	FThreadPool Pool(2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	TestTrue("Idle thread pool should remain running", Pool.IsRunning());
	TestEqual("Idle thread pool should keep explicit thread count", Pool.GetThreadCount(), 2u);
	return !Context.HasAnyFailures();
}

#undef ENIGMA_IMPLEMENT_THREAD_POOL_AUTOMATION_TEST

} // namespace Enigma
