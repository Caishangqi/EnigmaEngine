// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"
#include "Async/TaskHandle.h"

#include <functional>
#include <vector>

// -------------------------------------------------------------
// FTaskGraph
//
// DAG task scheduler. Tasks declare dependencies on other tasks;
// Dispatch() submits ready tasks to a FThreadPool (or executes
// them inline when running in single-threaded mode).
//
// Typical per-frame usage:
//   graph.CreateTask(workA);
//   auto hB = graph.CreateTask(workB, {hA});
//   graph.Dispatch();
//   graph.WaitAll();
//   graph.Reset();
// -------------------------------------------------------------

namespace Enigma
{

class FThreadPool;

class CORE_API FTaskGraph
{
public:
	/// Construct with a thread pool for parallel dispatch.
	explicit FTaskGraph(FThreadPool& threadPool);

	/// Construct in single-threaded mode (no thread pool).
	FTaskGraph();

	~FTaskGraph();

	// Non-copyable, non-movable
	FTaskGraph(const FTaskGraph&) = delete;
	FTaskGraph& operator=(const FTaskGraph&) = delete;

	/// Create a task with optional dependencies.
	/// The task will not execute until all prerequisites complete.
	FTaskHandle CreateTask(std::function<void()> work,
	                       std::vector<FTaskHandle> prerequisites = {});

	/// Dispatch all pending tasks respecting dependency order.
	/// In single-threaded mode, executes all tasks on the calling thread.
	void Dispatch();

	/// Block until all dispatched tasks complete.
	void WaitAll();

	/// Reset the graph for reuse (e.g. next frame).
	/// All previous task handles become invalid.
	void Reset();

	/// Returns true if running in single-threaded mode.
	bool IsSingleThreaded() const noexcept;

private:
	void dispatchSingleThreaded();
	void dispatchMultiThreaded();

	FThreadPool* m_threadPool = nullptr;
	std::vector<std::shared_ptr<TaskGraph_Private::FTaskNode>> m_nodes;
};

} // namespace Enigma
