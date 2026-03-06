// Copyright EnigmaEngine. All Rights Reserved.

/// @file SubsystemCollection.cpp
/// @brief Implementation of FSubsystemCollection.

#include "Subsystems/SubsystemCollection.h"

#include <algorithm>
#include <cstdio>

namespace Enigma
{

// -----------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------

void FSubsystemCollection::Initialize()
{
	if (bInitialized)
	{
		return;
	}

	// Phase 1: Create subsystems that pass ShouldCreateSubsystem
	for (const auto& reg : Registrations)
	{
		auto subsystem = reg.Factory();
		if (subsystem && subsystem->ShouldCreateSubsystem())
		{
			SubsystemMap.emplace(reg.Name, std::move(subsystem));
		}
	}

	// Phase 2: Initialize all created subsystems
	for (const auto& reg : Registrations)
	{
		InitializeSubsystem(reg.Name);
	}

	// Phase 3: PostInitialize all (in init order)
	for (auto* subsystem : InitOrder)
	{
		subsystem->PostInitialize();
	}

	// Phase 4: Build tick list (tickable subsystems sorted by priority desc)
	for (auto* subsystem : InitOrder)
	{
		if (subsystem->IsTickable())
		{
			TickList.push_back(subsystem);
		}
	}

	std::sort(TickList.begin(), TickList.end(),
		[](const ISubsystem* a, const ISubsystem* b)
		{
			return a->GetTickPriority() > b->GetTickPriority();
		});

	bInitialized = true;
}

void FSubsystemCollection::Tick(float deltaTime)
{
	for (auto* subsystem : TickList)
	{
		subsystem->Tick(deltaTime);
	}
}

void FSubsystemCollection::Deinitialize()
{
	if (!bInitialized)
	{
		return;
	}

	// Deinitialize in reverse initialization order
	for (auto it = InitOrder.rbegin(); it != InitOrder.rend(); ++it)
	{
		(*it)->Deinitialize();
	}

	TickList.clear();
	InitOrder.clear();
	SubsystemMap.clear();
	Registrations.clear();
	bInitialized = false;
}

bool FSubsystemCollection::IsInitialized() const
{
	return bInitialized;
}

void FSubsystemCollection::ForEachSubsystem(std::function<void(ISubsystem*)> operation) const
{
	for (const auto& pair : SubsystemMap)
	{
		operation(pair.second.get());
	}
}

// -----------------------------------------------------------------
// Internal
// -----------------------------------------------------------------

void FSubsystemCollection::InitializeSubsystem(const std::string& name)
{
	auto it = SubsystemMap.find(name);
	if (it == SubsystemMap.end())
	{
		return; // not created (ShouldCreateSubsystem was false, or not registered)
	}

	ISubsystem* subsystem = it->second.get();

	// Check if already initialized (present in InitOrder)
	for (const auto* existing : InitOrder)
	{
		if (existing == subsystem)
		{
			return; // already initialized
		}
	}

	// Initialize (may call InitializeDependency recursively)
	subsystem->Initialize(*this);
	InitOrder.push_back(subsystem);
}

} // namespace Enigma
