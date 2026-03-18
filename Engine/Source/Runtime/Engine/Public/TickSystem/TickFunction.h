// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EngineAPI.generated.h"
#include "TickSystem/TickGroup.h"

#include <cstdint>
#include <vector>

// -------------------------------------------------------------
// FTickFunction
//
// Abstract base class for tick registration units.
// Subclasses implement ExecuteTick() with actual tick logic.
// Follows UE5 FTickFunction virtual pattern.
//
// Configuration fields are public (set before registration).
// Registration is managed via FTickTaskManager.
// -------------------------------------------------------------

namespace Enigma
{

class FTickTaskManager;
class FComponent;

class ENGINE_API FTickFunction
{
public:
	virtual ~FTickFunction() = default;

	/// Pure virtual -- subclasses implement actual tick logic.
	virtual void ExecuteTick(float deltaTime) = 0;

	// ----- Registration -----

	/// Register this tick function with the given manager.
	void RegisterTickFunction(FTickTaskManager& manager);

	/// Unregister from the current manager.
	void UnregisterTickFunction();

	/// Returns true if currently registered.
	bool IsRegistered() const noexcept { return m_bRegistered; }

	// ----- Enable / Disable -----

	/// Enable or disable this tick function.
	/// Takes effect next frame (deferred).
	void SetTickFunctionEnable(bool bEnabled);

	/// Returns true if this tick function is enabled.
	bool IsTickFunctionEnabled() const noexcept { return m_bEnabled; }

	// ----- Prerequisites -----

	/// Declare that this tick function must execute after 'other'.
	/// Both must belong to the same tick group (cross-group ignored).
	void AddPrerequisite(FTickFunction& other);

	/// Remove a previously added prerequisite.
	void RemovePrerequisite(FTickFunction& other);

	/// Get the prerequisite list (read-only, for FTickTaskManager).
	const std::vector<FTickFunction*>& GetPrerequisites() const noexcept
	{
		return m_prerequisites;
	}

	/// Get the prerequisite list (mutable, for FTickTaskManager cleanup).
	std::vector<FTickFunction*>& GetPrerequisites() noexcept
	{
		return m_prerequisites;
	}

	// ----- Configuration (public, UE style -- set before registration) -----

	ETickGroup TickGroup          = ETickGroup::TG_Update;
	float      TickInterval       = 0.0f;   ///< 0 = every frame
	bool       bCanEverTick       = true;
	bool       bStartWithTickEnabled = true;
	bool       bRunOnAnyThread    = false;

	// ----- Internal state accessors (for FTickTaskManager) -----

	float  GetTickCooldownRemaining() const noexcept { return m_tickCooldownRemaining; }
	void   SetTickCooldownRemaining(float value) noexcept { m_tickCooldownRemaining = value; }
	FTickTaskManager* GetManager() const noexcept { return m_manager; }

private:
	bool               m_bRegistered = false;
	bool               m_bEnabled    = true;
	float              m_tickCooldownRemaining = 0.0f;
	FTickTaskManager*  m_manager     = nullptr;
	std::vector<FTickFunction*> m_prerequisites;

	friend class FTickTaskManager;
};

// -------------------------------------------------------------
// FComponentTickFunction
//
// Concrete FTickFunction for component ticking.
// Holds a raw pointer to the target FComponent and calls
// BeginPlay() (if needed) then Update() in ExecuteTick().
// -------------------------------------------------------------
class ENGINE_API FComponentTickFunction : public FTickFunction
{
public:
	FComponent* Target = nullptr;

	void ExecuteTick(float deltaTime) override;
};

} // namespace Enigma
