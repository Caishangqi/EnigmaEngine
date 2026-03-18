// Copyright EnigmaEngine. All Rights Reserved.

#include "Async/TaskGraph.h"
#include "Async/ThreadPool.h"
#include "Logging/LogMacros.h"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>

DEFINE_LOG_CATEGORY_STATIC(LogTaskGraph, Info, All);

namespace Enigma
{

// -----------------------------------------------------------------
// TaskNode -- internal node in the DAG
// -----------------------------------------------------------------
namespace TaskGraph_Private
{

struct FTaskNode
{
	std::function<void()>                    Work;
	std::atomic<int32_t>                     DependencyCount{0};
	std::vector<std::shared_ptr<FTaskNode>>  Dependents;
	std::atomic<bool>                        bComplete{false};
	std::mutex                               CompleteMutex;
	std::condition_variable                  CompleteCV;
};

} // namespace TaskGraph_Private

// -----------------------------------------------------------------
// FTaskHandle
// -----------------------------------------------------------------

FTaskHandle::FTaskHandle(std::shared_ptr<TaskGraph_Private::FTaskNode> node)
	: m_node(std::move(node))
{
}

bool FTaskHandle::IsComplete() const noexcept
{
	return m_node && m_node->bComplete.load(std::memory_order_acquire);
}

void FTaskHandle::Wait() const
{
	if (!m_node)
	{
		return;
	}

	std::unique_lock<std::mutex> lock(m_node->CompleteMutex);
	m_node->CompleteCV.wait(lock, [this]
	{
		return m_node->bComplete.load(std::memory_order_acquire);
	});
}

// -----------------------------------------------------------------
// FTaskGraph
// -----------------------------------------------------------------

FTaskGraph::FTaskGraph(FThreadPool& threadPool)
	: m_threadPool(&threadPool)
{
}

FTaskGraph::FTaskGraph()
	: m_threadPool(nullptr)
{
}

FTaskGraph::~FTaskGraph() = default;

FTaskHandle FTaskGraph::CreateTask(std::function<void()> work,
                                   std::vector<FTaskHandle> prerequisites)
{
	auto node = std::make_shared<TaskGraph_Private::FTaskNode>();
	node->Work = std::move(work);

	// Count valid prerequisites and wire up dependency edges
	int32_t depCount = 0;
	for (auto& prereq : prerequisites)
	{
		if (prereq.m_node && !prereq.m_node->bComplete.load(std::memory_order_acquire))
		{
			prereq.m_node->Dependents.push_back(node);
			++depCount;
		}
	}
	node->DependencyCount.store(depCount, std::memory_order_release);

	m_nodes.push_back(node);
	return FTaskHandle(node);
}

bool FTaskGraph::IsSingleThreaded() const noexcept
{
	return m_threadPool == nullptr;
}

void FTaskGraph::Dispatch()
{
	if (IsSingleThreaded())
	{
		dispatchSingleThreaded();
	}
	else
	{
		dispatchMultiThreaded();
	}
}

void FTaskGraph::WaitAll()
{
	for (auto& node : m_nodes)
	{
		std::unique_lock<std::mutex> lock(node->CompleteMutex);
		node->CompleteCV.wait(lock, [&node]
		{
			return node->bComplete.load(std::memory_order_acquire);
		});
	}
}

void FTaskGraph::Reset()
{
	m_nodes.clear();
}

// -----------------------------------------------------------------
// Single-threaded dispatch: topological order on calling thread
// -----------------------------------------------------------------
void FTaskGraph::dispatchSingleThreaded()
{
	// Collect nodes with zero dependencies first
	std::queue<std::shared_ptr<TaskGraph_Private::FTaskNode>> ready;
	for (auto& node : m_nodes)
	{
		if (node->DependencyCount.load(std::memory_order_acquire) == 0)
		{
			ready.push(node);
		}
	}

	while (!ready.empty())
	{
		auto current = std::move(ready.front());
		ready.pop();

		if (current->Work)
		{
			current->Work();
		}

		current->bComplete.store(true, std::memory_order_release);
		{
			std::lock_guard<std::mutex> lock(current->CompleteMutex);
			current->CompleteCV.notify_all();
		}

		for (auto& dep : current->Dependents)
		{
			if (dep->DependencyCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				ready.push(dep);
			}
		}
	}
}

// -----------------------------------------------------------------
// Multi-threaded dispatch: submit ready tasks to thread pool
// -----------------------------------------------------------------

static void completeAndPropagate(
	std::shared_ptr<TaskGraph_Private::FTaskNode> node,
	FThreadPool& pool);

static void executeAndPropagate(
	std::shared_ptr<TaskGraph_Private::FTaskNode> node,
	FThreadPool& pool)
{
	if (node->Work)
	{
		node->Work();
	}
	completeAndPropagate(std::move(node), pool);
}

static void completeAndPropagate(
	std::shared_ptr<TaskGraph_Private::FTaskNode> node,
	FThreadPool& pool)
{
	node->bComplete.store(true, std::memory_order_release);
	{
		std::lock_guard<std::mutex> lock(node->CompleteMutex);
		node->CompleteCV.notify_all();
	}

	for (auto& dep : node->Dependents)
	{
		if (dep->DependencyCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			auto ready = dep;
			pool.Submit([ready, &pool] { executeAndPropagate(ready, pool); });
		}
	}
}

void FTaskGraph::dispatchMultiThreaded()
{
	for (auto& node : m_nodes)
	{
		if (node->DependencyCount.load(std::memory_order_acquire) == 0)
		{
			auto readyNode = node;
			m_threadPool->Submit([readyNode, this]
			{
				executeAndPropagate(readyNode, *m_threadPool);
			});
		}
	}
}

} // namespace Enigma
