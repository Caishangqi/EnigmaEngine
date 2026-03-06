// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Subsystem.h
/// @brief Abstract base class for all engine subsystems.

#include "EngineAPI.generated.h"

#include <cstdint>

namespace Enigma
{

class FSubsystemCollection;

/// @brief Abstract base class for all engine subsystems.
///
/// Modeled after UE's USubsystem with simplified hierarchy.
///
/// UE equivalent: USubsystem (Engine/Source/Runtime/Engine/Public/Subsystems/Subsystem.h)
/// Key differences:
///   - UE uses UObject-based hierarchy (USubsystem -> UGameInstanceSubsystem, etc.)
///   - We use a flat ISubsystem with opt-in Tick (no UObject dependency)
///   - UE's PostInitialize() is on UWorldSubsystem only; we put it on the base class
class ENGINE_API ISubsystem
{
public:
	virtual ~ISubsystem() = default;

	/// Unique name for this subsystem class. Used as registry key in FSubsystemCollection.
	/// Each derived class MUST provide a static version: static const char* GetStaticName().
	/// Uses string comparison for now; future optimization path: hashed string for O(1) lookup.
	/// This avoids std::type_index which is unreliable across DLL boundaries.
	virtual const char* GetName() const = 0;

	/// Whether this subsystem should be created. Override to conditionally skip.
	/// UE equivalent: USubsystem::ShouldCreateSubsystem(UObject* Outer)
	virtual bool ShouldCreateSubsystem() const { return true; }

	/// Called once after creation. Use collection to resolve dependencies.
	/// UE equivalent: USubsystem::Initialize(FSubsystemCollectionBase& Collection)
	virtual void Initialize(FSubsystemCollection& collection) {}

	/// Called after ALL subsystems have been initialized.
	/// UE equivalent: UWorldSubsystem::PostInitialize() (world subsystem only in UE)
	virtual void PostInitialize() {}

	/// Called each frame. Only invoked if IsTickable() returns true.
	/// UE equivalent: UTickableWorldSubsystem::Tick(float DeltaTime)
	virtual void Tick(float deltaTime) {}

	/// Whether this subsystem participates in per-frame ticking.
	/// UE equivalent: FTickableGameObject::GetTickableTickType()
	virtual bool IsTickable() const { return false; }

	/// Tick priority (higher = earlier). Used for ordering among tickable subsystems.
	/// UE equivalent: FTickFunction::bHighPriority + TickGroup
	virtual int32_t GetTickPriority() const { return 0; }

	/// Called during shutdown, in reverse initialization order.
	/// UE equivalent: USubsystem::Deinitialize()
	virtual void Deinitialize() {}
};

} // namespace Enigma
