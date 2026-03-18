// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

// -------------------------------------------------------------
// FThreadPool
//
// General-purpose worker thread pool.
// Workers sleep on a condition variable when idle (no busy-wait).
// Submit() returns a std::future for the task result.
// Shutdown() drains the queue and joins all threads.
//
// Usage:
//   FThreadPool pool;                       // auto thread count
//   auto f = pool.Submit([]{ return 42; }); // submit work
//   int r = f.get();                        // wait for result
//   pool.Shutdown();                        // explicit shutdown
// -------------------------------------------------------------

namespace Enigma
{

class CORE_API FThreadPool
{
public:
	/// Construct with explicit thread count.
	/// @param numThreads  Number of worker threads. 0 = hardware_concurrency - 1 (min 1).
	explicit FThreadPool(uint32_t numThreads = 0);

	/// Destructor -- calls Shutdown() if not already shut down.
	~FThreadPool();

	// Non-copyable, non-movable
	FThreadPool(const FThreadPool&) = delete;
	FThreadPool& operator=(const FThreadPool&) = delete;
	FThreadPool(FThreadPool&&) = delete;
	FThreadPool& operator=(FThreadPool&&) = delete;

	/// Submit a callable task. Returns a future for the result.
	/// Thread-safe: may be called from any thread.
	template <typename F>
	auto Submit(F&& task) -> std::future<std::invoke_result_t<F>>
	{
		using ReturnType = std::invoke_result_t<F>;

		auto packaged = std::make_shared<std::packaged_task<ReturnType()>>(
			std::forward<F>(task));
		std::future<ReturnType> result = packaged->get_future();

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_tasks.emplace([packaged]() { (*packaged)(); });
		}
		m_condition.notify_one();

		return result;
	}

	/// Query the number of worker threads.
	uint32_t GetThreadCount() const noexcept;

	/// Returns true if the pool is running (not shut down).
	bool IsRunning() const noexcept;

	/// Drain the task queue and join all worker threads.
	/// Safe to call multiple times (subsequent calls are no-ops).
	void Shutdown();

private:
	void workerLoop();

	std::vector<std::thread>            m_workers;
	std::queue<std::function<void()>>   m_tasks;
	mutable std::mutex                  m_mutex;
	std::condition_variable             m_condition;
	bool                                m_bStopping = false;
};

} // namespace Enigma
