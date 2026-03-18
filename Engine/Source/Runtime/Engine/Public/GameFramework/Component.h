// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Component.h
/// @brief Abstract base class for all components attached to FGameObject.

#include "EngineAPI.generated.h"
#include "Misc/Name.h"
#include "TickSystem/TickGroup.h"

#include <memory>

namespace Enigma
{

class FGameObject;
class FComponentTickFunction;

/// @brief Abstract base class for all components.
///
/// Defines the component lifecycle contract with two-phase initialization:
///   1. OnAttach()  -- called immediately when added to a FGameObject (self-init, no cross-component deps)
///   2. BeginPlay() -- called once before the first Update (safe to access sibling components)
///   3. Update()    -- called every frame on active, enabled components
///   4. OnDetach()  -- called when removed from a FGameObject or when the owner is destroyed
///
/// Derived classes MUST provide: static FName GetStaticName()
///
/// No Render() on base class -- rendering is FRenderComponent's responsibility.
///
/// UE equivalent: UActorComponent (simplified)
/// Unity equivalent: Component + MonoBehaviour (merged)
class ENGINE_API FComponent
{
public:
	FComponent();
	virtual ~FComponent();

	// ----- Type Identification -----

	/// Derived classes MUST define: static FName GetStaticName()
	/// Returns the component type name for O(1) lookup via FName index comparison.
	/// Example:
	///   static FName GetStaticName() { return FName("PlayerController"); }
	virtual FName GetName() const = 0;

	// ----- Lifecycle Hooks -----

	/// Called immediately when this component is attached to a FGameObject.
	/// Use for self-initialization only (no cross-component dependencies).
	/// Unity equivalent: Awake()
	/// @param owner The FGameObject this component is being attached to.
	virtual void OnAttach(FGameObject* owner);

	/// Called once before the first Update(), after all components are attached.
	/// Safe to call GetOwner()->GetComponent<T>() here.
	/// Unity equivalent: Start()
	/// Protected by m_bBegunPlay flag -- will not be called twice.
	virtual void BeginPlay();

	/// Called every frame on active, enabled components.
	/// Unity equivalent: Update()
	/// @param deltaTime Time elapsed since last frame, in seconds.
	virtual void Update(float deltaTime);

	/// Called when this component is detached from its owner.
	/// Unity equivalent: OnDestroy()
	virtual void OnDetach();

	// ----- Enable / Disable -----

	/// Check if this component is enabled. Disabled components skip Update().
	[[nodiscard]] bool IsEnabled() const noexcept { return m_bEnabled; }

	/// Enable or disable this component.
	/// Also syncs the tick function enable state if present.
	void SetEnabled(bool bEnabled);

	// ----- Owner Access -----

	/// Get the FGameObject this component is attached to.
	/// Returns nullptr if not attached.
	[[nodiscard]] FGameObject* GetOwner() const noexcept { return m_owner; }

	// ----- BeginPlay State -----

	/// Check if BeginPlay() has been called on this component.
	[[nodiscard]] bool HasBegunPlay() const noexcept { return m_bBegunPlay; }

	// ----- Tick Configuration (set in constructor, before OnAttach) -----

	/// If true, a FComponentTickFunction is created and registered on attach.
	/// Default: false (zero overhead for non-ticking components).
	bool bCanEverTick = false;

	/// If false, the tick function is registered but disabled until explicitly enabled.
	bool bStartWithTickEnabled = true;

	/// Which tick group this component belongs to.
	ETickGroup TickGroup = ETickGroup::TG_Update;

	/// Tick interval in seconds. 0 = every frame.
	float TickInterval = 0.0f;

	// ----- Tick Function Access -----

	/// Returns the tick function, or nullptr if bCanEverTick is false.
	[[nodiscard]] FComponentTickFunction* GetTickFunction() const noexcept
	{
		return m_tickFunction.get();
	}

protected:
	FGameObject* m_owner = nullptr;
	bool m_bEnabled = true;
	bool m_bBegunPlay = false;
	std::unique_ptr<FComponentTickFunction> m_tickFunction;
};

} // namespace Enigma
