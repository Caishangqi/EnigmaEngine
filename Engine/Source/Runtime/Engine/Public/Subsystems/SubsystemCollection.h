// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file SubsystemCollection.h
/// @brief Manages subsystem lifecycle: creation, initialization, ticking, shutdown.

#include "EngineAPI.generated.h"
#include "Subsystems/Subsystem.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Enigma
{

/// @brief Manages subsystem lifecycle: creation, initialization, ticking, shutdown.
///
/// UE equivalent: FSubsystemCollectionBase + FSubsystemCollection<T>
///   (Engine/Source/Runtime/Engine/Public/Subsystems/SubsystemCollection.h)
/// Key differences:
///   - UE uses UClass* as key with TMap<UClass*, USubsystem*> storage
///   - We use string-based registration (static name per subsystem class) for cross-DLL safety
///   - std::type_index was considered but rejected for cross-DLL RTTI issues
///   - Each subsystem provides a static name via GetStaticName()
///   - UE has ActivateExternalSubsystem/DeactivateExternalSubsystem for plugin hot-loading
///   - We omit GC integration (no UObject)
class ENGINE_API FSubsystemCollection
{
public:
	FSubsystemCollection() = default;
	~FSubsystemCollection() = default;

	// Non-copyable
	FSubsystemCollection(const FSubsystemCollection&) = delete;
	FSubsystemCollection& operator=(const FSubsystemCollection&) = delete;

	/// Register a subsystem type. Called during module startup.
	/// Internally calls T::GetStaticName() to obtain the unique key.
	template <typename T>
	void RegisterSubsystem();

	/// Initialize all registered subsystems (filter, create, init, post-init).
	/// UE equivalent: FSubsystemCollectionBase::Initialize(UObject* NewOuter)
	void Initialize();

	/// Tick all tickable subsystems in priority order.
	void Tick(float deltaTime);

	/// Deinitialize all subsystems in reverse order.
	/// UE equivalent: FSubsystemCollectionBase::Deinitialize()
	void Deinitialize();

	/// Whether the collection has been initialized.
	/// UE equivalent: FSubsystemCollectionBase::IsInitialized()
	bool IsInitialized() const;

	/// Retrieve a subsystem by type. Returns nullptr if not found.
	/// Internally uses T::GetStaticName() for lookup.
	/// UE equivalent: FSubsystemCollection<T>::GetSubsystem<T>()
	template <typename T>
	T* GetSubsystem() const;

	/// Ensure a dependency is initialized before the caller.
	/// Called from within ISubsystem::Initialize().
	/// UE equivalent: FSubsystemCollectionBase::InitializeDependency<T>()
	template <typename T>
	T* InitializeDependency();

	/// Iterate over all subsystems.
	/// UE equivalent: FSubsystemCollectionBase::ForEachSubsystem()
	void ForEachSubsystem(std::function<void(ISubsystem*)> operation) const;

private:
	/// Internal registration entry (pre-initialization).
	struct FRegistration
	{
		std::string Name;
		std::function<std::unique_ptr<ISubsystem>()> Factory;
	};

	/// Initialize a single subsystem by name. Used by InitializeDependency.
	void InitializeSubsystem(const std::string& name);

	std::vector<FRegistration> Registrations;

	// Active subsystem storage: string name -> subsystem instance
	// Uses std::string as key for cross-DLL safety (no RTTI dependency).
	std::unordered_map<std::string, std::unique_ptr<ISubsystem>> SubsystemMap;
	std::vector<ISubsystem*> InitOrder;    // tracks initialization order for reverse shutdown
	std::vector<ISubsystem*> TickList;     // sorted by priority, rebuilt on init
	bool bInitialized = false;
};

// ---------------------------------------------------------------
// Template implementations
// ---------------------------------------------------------------

template <typename T>
void FSubsystemCollection::RegisterSubsystem()
{
	const char* name = T::GetStaticName();

	// Check for duplicate registration
	for (const auto& reg : Registrations)
	{
		if (reg.Name == name)
		{
			return; // silently ignore duplicate
		}
	}

	Registrations.push_back({
		std::string(name),
		[]() -> std::unique_ptr<ISubsystem> { return std::make_unique<T>(); }
	});
}

template <typename T>
T* FSubsystemCollection::GetSubsystem() const
{
	const char* name = T::GetStaticName();
	auto it = SubsystemMap.find(name);
	if (it != SubsystemMap.end())
	{
		return static_cast<T*>(it->second.get());
	}
	return nullptr;
}

template <typename T>
T* FSubsystemCollection::InitializeDependency()
{
	const char* name = T::GetStaticName();
	InitializeSubsystem(name);
	return GetSubsystem<T>();
}

} // namespace Enigma
