// Copyright EnigmaEngine. All Rights Reserved.

#include "Async/ThreadPool.h"
#include "Logging/LogMacros.h"

#include <algorithm>

DEFINE_LOG_CATEGORY_STATIC(LogThreadPool, Info, All);

namespace Enigma
{

FThreadPool::FThreadPool(uint32_t numThreads)
{
	if (numThreads == 0)
	{
		const uint32_t hw = std::thread::hardware_concurrency();
		numThreads = (hw > 1) ? (hw - 1) : 1;
	}

	ENIGMA_LOG(LogThreadPool, Info, "Creating thread pool with {} workers", numThreads);

	m_workers.reserve(numThreads);
	for (uint32_t i = 0; i < numThreads; ++i)
	{
		m_workers.emplace_back(&FThreadPool::workerLoop, this);
	}
}

FThreadPool::~FThreadPool()
{
	Shutdown();
}

uint32_t FThreadPool::GetThreadCount() const noexcept
{
	return static_cast<uint32_t>(m_workers.size());
}

bool FThreadPool::IsRunning() const noexcept
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return !m_bStopping;
}

void FThreadPool::Shutdown()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_bStopping)
		{
			return; // already shut down
		}
		m_bStopping = true;
	}

	m_condition.notify_all();

	for (auto& worker : m_workers)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}

	ENIGMA_LOG(LogThreadPool, Info, "Thread pool shut down ({} workers joined)",
		m_workers.size());
}

void FThreadPool::workerLoop()
{
	for (;;)
	{
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_condition.wait(lock, [this]
			{
				return m_bStopping || !m_tasks.empty();
			});

			if (m_bStopping && m_tasks.empty())
			{
				return;
			}

			task = std::move(m_tasks.front());
			m_tasks.pop();
		}

		task();
	}
}

} // namespace Enigma
