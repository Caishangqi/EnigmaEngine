// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EngineAPI.generated.h"
#include "Subsystems/Subsystem.h"
#include "TickSystem/TickGroup.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

// -------------------------------------------------------------
// FTickTaskManager
//
// ISubsystem-derived tick scheduling subsystem.
// Manages FTickFunction registration, tick group ordering,
// prerequisite dependencies, and optional parallel dispatch
// via FTaskGraph + FThreadPool.
//
// Registered via FSubsystemCollection in EngineModule startup.
// TickPriority = 500 (lower than FInputSubsystem at 1000,
// ensuring input is processed before tick dispatch).
// -------------------------------------------------------------

namespace Enigma
{

class FTickFunction;
class FThreadPool;
class FTaskGraph;

class ENGINE_API FTickTaskManager : public ISubsystem
{
public:
	FTickTaskManager();
	~FTickTaskManager() override;
	// ----- ISubsystem interface -----

	static const char* GetStaticName() { return "FTickTaskManager"; }
	const char* GetName() const override { return GetStaticName(); }
	bool IsTickable() const override { return true; }
	int32_t GetTickPriority() const override { return 500; }

	void Initialize(FSubsystemCollection& collection) override;
	void Tick(float deltaTime) override;
	void Deinitialize() override;

	// ----- Tick Function Management -----

	/// Add a tick function to the appropriate group list.
	void AddTickFunction(FTickFunction& tickFunc);

	/// Remove a tick function from all lists and clean prerequisite references.
	void RemoveTickFunction(FTickFunction& tickFunc);

	/// Apply pending tick registration changes without executing any tick functions.
	void FlushPendingChanges();

private:
	/// Per-group tick function lists.
	struct FTickGroupData
	{
		std::vector<FTickFunction*> EnabledTicks;
		std::vector<FTickFunction*> DisabledTicks;
	};
	std::array<FTickGroupData, static_cast<size_t>(ETickGroup::TG_Count)> m_tickGroups;

	/// Deferred add/remove queues (applied at frame boundary).
	std::vector<FTickFunction*> m_pendingAdds;

	/// Pending removes store pointer + tick group to avoid dereferencing
	/// a potentially destroyed tick function during processPendingChanges.
	struct FPendingRemove
	{
		FTickFunction* Func;
		ETickGroup Group;
	};
	std::vector<FPendingRemove> m_pendingRemoves;

	/// TaskGraph integration.
	std::unique_ptr<FThreadPool> m_threadPool;
	std::unique_ptr<FTaskGraph>  m_taskGraph;
	bool m_bSingleThreaded = false;

	/// Process deferred additions and removals.
	void processPendingChanges();

	/// Execute all tick functions in a single tick group.
	void executeTickGroup(ETickGroup group, float deltaTime);

	/// DFS cycle detection: returns true if adding 'to' as prerequisite of 'from' creates a cycle.
	bool detectCycle(const FTickFunction& from, const FTickFunction& to) const;

	/// Remove 'target' from all prerequisite lists across all groups.
	void cleanPrerequisiteReferences(FTickFunction* target);
};

} // namespace Enigma
