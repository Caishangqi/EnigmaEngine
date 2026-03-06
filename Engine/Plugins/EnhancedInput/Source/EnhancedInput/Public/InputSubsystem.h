// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputSubsystem.h
/// @brief Core input subsystem: manages mapping contexts, processes input pipeline, dispatches callbacks.

#include "InputKeys.h"  // for ENHANCEDINPUT_API, FKey
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "InputMappingContext.h"
#include "Subsystems/Subsystem.h"
#include "Delegates/Delegate.h"
#include "Delegates/DelegateHandle.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Enigma
{

/// @brief Per-action runtime state (tracks trigger state across frames).
///
/// UE equivalent: FInputActionInstance (EnhancedInput/Public/InputAction.h)
struct FInputActionInstance
{
	const FInputAction* SourceAction = nullptr;
	FInputActionValue Value;
	ETriggerState LastTriggerState = ETriggerState::None;
	ETriggerEvent LastTriggerEvent = ETriggerEvent::None;
	float ElapsedTime = 0.0f;    // time in Started/Ongoing/Triggered
	float TriggeredTime = 0.0f;  // time in Triggered only
};

/// @brief Callback signature for action bindings.
using FInputActionCallback = TDelegate<void(const FInputActionInstance&)>;

/// @brief Binding handle for removal.
struct FInputBindingHandle
{
	FDelegateHandle Handle;
	const FInputAction* Action = nullptr;
	ETriggerEvent Event = ETriggerEvent::None;
};

/// @brief The main input subsystem. First ISubsystem implementation.
class ENHANCEDINPUT_API FInputSubsystem : public ISubsystem
{
public:
	// --- ISubsystem interface ---
	static const char* GetStaticName() { return "FInputSubsystem"; }
	const char* GetName() const override { return GetStaticName(); }
	bool IsTickable() const override { return true; }
	int32_t GetTickPriority() const override { return 1000; }
	void Initialize(FSubsystemCollection& collection) override;
	void Deinitialize() override;
	void Tick(float deltaTime) override;

	// --- Mapping Context Management ---
	void AddMappingContext(const FInputMappingContext* context, int32_t priority);
	void RemoveMappingContext(const FInputMappingContext* context);
	void ClearAllMappingContexts();

	// --- Action Binding ---
	FInputBindingHandle BindAction(
		const FInputAction* action,
		ETriggerEvent triggerEvent,
		FInputActionCallback callback);

	bool UnbindAction(const FInputBindingHandle& handle);
	void ClearBindings();

	// --- Key State (fed by FInputMessageBridge) ---
	void SetKeyState(FKey Key, bool bPressed);
	void SetAxisValue(FKey Key, float Value);
	bool GetKeyState(FKey Key) const;

	// --- Query ---
	const FInputActionInstance* GetActionInstance(const FInputAction* action) const;

private:
	void rebuildMappingTable();
	void processInput(float deltaTime);
	ETriggerEvent determineEvent(ETriggerState previous, ETriggerState current);

	struct FActiveContext
	{
		const FInputMappingContext* Context = nullptr;
		int32_t Priority = 0;
	};
	std::vector<FActiveContext> activeContexts; // sorted by priority desc

	struct FBinding
	{
		FDelegateHandle Handle;
		const FInputAction* Action = nullptr;
		ETriggerEvent Event = ETriggerEvent::None;
		FInputActionCallback Callback;
	};
	std::vector<FBinding> bindings;

	std::unordered_map<FKey, bool, FKey::Hash> keyStates;
	std::unordered_map<FKey, float, FKey::Hash> axisValues;
	std::unordered_map<const FInputAction*, FInputActionInstance> actionInstances;

	// Flattened mapping table (rebuilt on context add/remove)
	struct FResolvedMapping
	{
		const FInputAction* Action = nullptr;
		FKey Key;
		std::vector<IInputModifier*> Modifiers; // mapping + action merged
		std::vector<IInputTrigger*> Triggers;   // mapping or action
		int32_t ContextPriority = 0;
	};
	std::vector<FResolvedMapping> resolvedMappings;
	bool bMappingsDirty = true;

	// Default trigger used when no triggers are specified
	FInputTriggerDown defaultTrigger;
};

} // namespace Enigma
