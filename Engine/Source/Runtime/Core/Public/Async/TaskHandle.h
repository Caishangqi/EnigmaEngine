// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

#include <atomic>
#include <cstdint>
#include <memory>

// -------------------------------------------------------------
// FTaskHandle
//
// Lightweight handle to a task submitted to FTaskGraph.
// Wraps a shared_ptr to an internal node; copyable and movable.
// IsComplete() is lock-free (atomic load).
// Wait() blocks until the task finishes.
// -------------------------------------------------------------

namespace Enigma
{

namespace TaskGraph_Private
{
	struct FTaskNode;
}

class CORE_API FTaskHandle
{
public:
	FTaskHandle() = default;

	/// Returns true if the task has finished executing.
	bool IsComplete() const noexcept;

	/// Block the calling thread until the task completes.
	void Wait() const;

	/// Returns true if this handle refers to a valid task.
	bool IsValid() const noexcept { return m_node != nullptr; }

private:
	friend class FTaskGraph;
	explicit FTaskHandle(std::shared_ptr<TaskGraph_Private::FTaskNode> node);

	std::shared_ptr<TaskGraph_Private::FTaskNode> m_node;
};

} // namespace Enigma
