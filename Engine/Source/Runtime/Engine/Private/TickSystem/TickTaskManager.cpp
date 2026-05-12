// Copyright EnigmaEngine. All Rights Reserved.

#include "TickSystem/TickTaskManager.h"
#include "TickSystem/TickFunction.h"
#include "Async/ThreadPool.h"
#include "Async/TaskGraph.h"
#include "Misc/ConfigCacheIni.h"
#include "CoreGlobals.h"
#include "Logging/LogMacros.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

DEFINE_LOG_CATEGORY_STATIC(LogTickSystem, Info, All);

namespace Enigma
{

FTickTaskManager::FTickTaskManager() = default;
FTickTaskManager::~FTickTaskManager() = default;

void FTickTaskManager::Initialize(FSubsystemCollection& collection)
{
	// Read config
	if (GConfig)
	{
		GConfig->GetBool("/Script/Engine.TickSystem", "bSingleThreaded", m_bSingleThreaded, "Engine");

		int32_t workerCount = 0;
		GConfig->GetInt("/Script/Engine.TickSystem", "WorkerThreadCount", workerCount, "Engine");

		if (!m_bSingleThreaded)
		{
			m_threadPool = std::make_unique<FThreadPool>(
				static_cast<uint32_t>(workerCount));
			m_taskGraph = std::make_unique<FTaskGraph>(*m_threadPool);
		}
	}

	if (m_bSingleThreaded)
	{
		ENIGMA_LOG(LogTickSystem, Info, "FTickTaskManager initialized (single-threaded mode)");
	}
	else
	{
		ENIGMA_LOG(LogTickSystem, Info, "FTickTaskManager initialized ({} worker threads)",
			m_threadPool ? m_threadPool->GetThreadCount() : 0);
	}
}

void FTickTaskManager::Tick(float deltaTime)
{
	processPendingChanges();

	executeTickGroup(ETickGroup::TG_PreUpdate, deltaTime);
	executeTickGroup(ETickGroup::TG_Update, deltaTime);
	executeTickGroup(ETickGroup::TG_PostUpdate, deltaTime);
}

void FTickTaskManager::Deinitialize()
{
	for (auto& group : m_tickGroups)
	{
		group.EnabledTicks.clear();
		group.DisabledTicks.clear();
	}
	m_pendingAdds.clear();
	m_pendingRemoves.clear();

	m_taskGraph.reset();
	m_threadPool.reset();

	ENIGMA_LOG(LogTickSystem, Info, "FTickTaskManager deinitialized");
}

void FTickTaskManager::AddTickFunction(FTickFunction& tickFunc)
{
	m_pendingAdds.push_back(&tickFunc);
}

void FTickTaskManager::RemoveTickFunction(FTickFunction& tickFunc)
{
	m_pendingRemoves.push_back({&tickFunc, tickFunc.TickGroup});
}

void FTickTaskManager::FlushPendingChanges()
{
	processPendingChanges();
}

// -----------------------------------------------------------------
// processPendingChanges -- apply deferred adds/removes at frame boundary
// -----------------------------------------------------------------
void FTickTaskManager::processPendingChanges()
{
	// Process removals first (use stored TickGroup to avoid dereferencing
	// potentially destroyed tick functions)
	for (auto& pending : m_pendingRemoves)
	{
		cleanPrerequisiteReferences(pending.Func);

		// Also remove from pending adds (add+remove in same frame)
		auto itAdd = std::find(m_pendingAdds.begin(), m_pendingAdds.end(), pending.Func);
		if (itAdd != m_pendingAdds.end())
		{
			m_pendingAdds.erase(itAdd);
		}

		const auto groupIdx = static_cast<size_t>(pending.Group);
		if (groupIdx < m_tickGroups.size())
		{
			auto& group = m_tickGroups[groupIdx];

			auto itE = std::find(group.EnabledTicks.begin(), group.EnabledTicks.end(), pending.Func);
			if (itE != group.EnabledTicks.end())
			{
				group.EnabledTicks.erase(itE);
			}

			auto itD = std::find(group.DisabledTicks.begin(), group.DisabledTicks.end(), pending.Func);
			if (itD != group.DisabledTicks.end())
			{
				group.DisabledTicks.erase(itD);
			}
		}
	}
	m_pendingRemoves.clear();

	// Process additions
	for (auto* tf : m_pendingAdds)
	{
		const auto groupIdx = static_cast<size_t>(tf->TickGroup);
		if (groupIdx >= m_tickGroups.size())
		{
			ENIGMA_LOG(LogTickSystem, Error, "Invalid tick group index {}", groupIdx);
			continue;
		}

		auto& group = m_tickGroups[groupIdx];
		if (tf->IsTickFunctionEnabled())
		{
			group.EnabledTicks.push_back(tf);
		}
		else
		{
			group.DisabledTicks.push_back(tf);
		}
	}
	m_pendingAdds.clear();

	// Sync enable/disable state changes (move between lists)
	for (auto& group : m_tickGroups)
	{
		// Move newly disabled from Enabled to Disabled
		for (auto it = group.EnabledTicks.begin(); it != group.EnabledTicks.end(); )
		{
			if (!(*it)->IsTickFunctionEnabled())
			{
				group.DisabledTicks.push_back(*it);
				it = group.EnabledTicks.erase(it);
			}
			else
			{
				++it;
			}
		}

		// Move newly enabled from Disabled to Enabled
		for (auto it = group.DisabledTicks.begin(); it != group.DisabledTicks.end(); )
		{
			if ((*it)->IsTickFunctionEnabled())
			{
				group.EnabledTicks.push_back(*it);
				it = group.DisabledTicks.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}

// -----------------------------------------------------------------
// executeTickGroup -- run all enabled tick functions in one group
// -----------------------------------------------------------------
void FTickTaskManager::executeTickGroup(ETickGroup group, float deltaTime)
{
	const auto groupIdx = static_cast<size_t>(group);
	auto& ticks = m_tickGroups[groupIdx].EnabledTicks;

	if (ticks.empty())
	{
		return;
	}

	// Execute a single tick function (with cooldown check)
	auto executeSingleTick = [deltaTime](FTickFunction* tf)
	{
		if (tf->TickInterval > 0.0f)
		{
			float remaining = tf->GetTickCooldownRemaining() - deltaTime;
			if (remaining > 0.0f)
			{
				tf->SetTickCooldownRemaining(remaining);
				return;
			}
			tf->SetTickCooldownRemaining(tf->TickInterval);
		}

		tf->ExecuteTick(deltaTime);
	};

	if (m_bSingleThreaded || !m_taskGraph)
	{
		// Single-threaded: topological sort by prerequisites, then execute
		std::unordered_set<FTickFunction*> groupSet(ticks.begin(), ticks.end());
		std::unordered_map<FTickFunction*, int32_t> inDegree;
		std::unordered_map<FTickFunction*, std::vector<FTickFunction*>> dependents;

		for (auto* tf : ticks)
		{
			inDegree[tf] = 0;
		}

		for (auto* tf : ticks)
		{
			for (auto* prereq : tf->GetPrerequisites())
			{
				if (groupSet.count(prereq))
				{
					inDegree[tf]++;
					dependents[prereq].push_back(tf);
				}
			}
		}

		// Kahn's algorithm
		std::queue<FTickFunction*> ready;
		for (auto* tf : ticks)
		{
			if (inDegree[tf] == 0)
			{
				ready.push(tf);
			}
		}

		while (!ready.empty())
		{
			auto* current = ready.front();
			ready.pop();

			executeSingleTick(current);

			for (auto* dep : dependents[current])
			{
				if (--inDegree[dep] == 0)
				{
					ready.push(dep);
				}
			}
		}
	}
	else
	{
		// Multi-threaded: build TaskGraph tasks with prerequisite edges
		m_taskGraph->Reset();

		std::unordered_set<FTickFunction*> groupSet(ticks.begin(), ticks.end());
		std::unordered_map<FTickFunction*, FTaskHandle> handleMap;

		for (auto* tf : ticks)
		{
			std::vector<FTaskHandle> prereqHandles;
			for (auto* prereq : tf->GetPrerequisites())
			{
				if (groupSet.count(prereq))
				{
					auto it = handleMap.find(prereq);
					if (it != handleMap.end())
					{
						prereqHandles.push_back(it->second);
					}
				}
			}

			auto handle = m_taskGraph->CreateTask(
				[executeSingleTick, tf] { executeSingleTick(tf); },
				std::move(prereqHandles));

			handleMap[tf] = handle;
		}

		m_taskGraph->Dispatch();
		m_taskGraph->WaitAll();
	}
}

// -----------------------------------------------------------------
// detectCycle -- BFS from 'to' following prerequisites; true if 'from' reachable
// -----------------------------------------------------------------
bool FTickTaskManager::detectCycle(const FTickFunction& from, const FTickFunction& to) const
{
	std::unordered_set<const FTickFunction*> visited;
	std::queue<const FTickFunction*> queue;
	queue.push(&to);

	while (!queue.empty())
	{
		const auto* current = queue.front();
		queue.pop();

		if (current == &from)
		{
			return true;
		}

		if (visited.count(current))
		{
			continue;
		}
		visited.insert(current);

		for (const auto* prereq : current->GetPrerequisites())
		{
			queue.push(prereq);
		}
	}

	return false;
}

// -----------------------------------------------------------------
// cleanPrerequisiteReferences -- remove target from all prerequisite lists
// -----------------------------------------------------------------
void FTickTaskManager::cleanPrerequisiteReferences(FTickFunction* target)
{
	// Remove target from all prerequisite lists using pointer comparison only.
	// target may already be destroyed, so we must not dereference it.
	for (auto& group : m_tickGroups)
	{
		for (auto* tf : group.EnabledTicks)
		{
			auto& prereqs = tf->GetPrerequisites();
			auto it = std::find(prereqs.begin(), prereqs.end(), target);
			if (it != prereqs.end())
			{
				prereqs.erase(it);
			}
		}
		for (auto* tf : group.DisabledTicks)
		{
			auto& prereqs = tf->GetPrerequisites();
			auto it = std::find(prereqs.begin(), prereqs.end(), target);
			if (it != prereqs.end())
			{
				prereqs.erase(it);
			}
		}
	}
}

} // namespace Enigma
