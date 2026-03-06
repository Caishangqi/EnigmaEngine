// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputSubsystem.cpp
/// @brief Implementation of FInputSubsystem — the core input processing pipeline.

#include "InputSubsystem.h"
#include "InputMessageBridge.h"
#include "Engine/Engine.h"
#include "GameFramework/GameInstance.h"
#include "GenericPlatform/GenericApplication.h"
#include "Logging/LogMacros.h"
#include "Logging/LogCategory.h"

#include <algorithm>
#include <cmath>

DEFINE_LOG_CATEGORY_STATIC(LogTemp, Info, All);

namespace Enigma
{

// -----------------------------------------------------------------
// ISubsystem lifecycle
// -----------------------------------------------------------------

void FInputSubsystem::Initialize(FSubsystemCollection& /*collection*/)
{
	ENIGMA_LOG(LogTemp, Info, "[FInputSubsystem] Initialize");

	// Install FInputMessageBridge as the application's message handler
	FGenericApplication* app = FGenericApplication::GetApplication();
	if (app)
	{
		FGenericApplicationMessageHandler* oldHandler = app->GetMessageHandler();
		auto* bridge = new FInputMessageBridge(this, oldHandler);
		app->SetMessageHandler(bridge);
		ENIGMA_LOG(LogTemp, Info, "[FInputSubsystem] InputMessageBridge installed");
	}
}

void FInputSubsystem::Deinitialize()
{
	ClearBindings();
	ClearAllMappingContexts();
	keyStates.clear();
	axisValues.clear();
	actionInstances.clear();
	resolvedMappings.clear();
	ENIGMA_LOG(LogTemp, Info, "[FInputSubsystem] Deinitialize");
}

void FInputSubsystem::Tick(float deltaTime)
{
	if (bMappingsDirty)
	{
		rebuildMappingTable();
		bMappingsDirty = false;
	}
	processInput(deltaTime);
}

// -----------------------------------------------------------------
// Mapping Context Management
// -----------------------------------------------------------------

void FInputSubsystem::AddMappingContext(const FInputMappingContext* context, int32_t priority)
{
	if (!context)
	{
		return;
	}

	// Remove if already present
	RemoveMappingContext(context);

	activeContexts.push_back({context, priority});

	// Sort by priority descending
	std::sort(activeContexts.begin(), activeContexts.end(),
		[](const FActiveContext& a, const FActiveContext& b)
		{
			return a.Priority > b.Priority;
		});

	bMappingsDirty = true;
}

void FInputSubsystem::RemoveMappingContext(const FInputMappingContext* context)
{
	activeContexts.erase(
		std::remove_if(activeContexts.begin(), activeContexts.end(),
			[context](const FActiveContext& ac) { return ac.Context == context; }),
		activeContexts.end());
	bMappingsDirty = true;
}

void FInputSubsystem::ClearAllMappingContexts()
{
	activeContexts.clear();
	bMappingsDirty = true;
}

// -----------------------------------------------------------------
// Action Binding
// -----------------------------------------------------------------

FInputBindingHandle FInputSubsystem::BindAction(
	const FInputAction* action,
	ETriggerEvent triggerEvent,
	FInputActionCallback callback)
{
	FInputBindingHandle handle;
	if (!action)
	{
		ENIGMA_LOG(LogTemp, Warning, "[FInputSubsystem] BindAction: null action");
		return handle;
	}

	handle.Handle = FDelegateHandle::Generate();
	handle.Action = action;
	handle.Event = triggerEvent;

	FBinding binding;
	binding.Handle = handle.Handle;
	binding.Action = action;
	binding.Event = triggerEvent;
	binding.Callback = std::move(callback);
	bindings.push_back(std::move(binding));

	return handle;
}

bool FInputSubsystem::UnbindAction(const FInputBindingHandle& handle)
{
	auto it = std::remove_if(bindings.begin(), bindings.end(),
		[&](const FBinding& b) { return b.Handle == handle.Handle; });
	if (it != bindings.end())
	{
		bindings.erase(it, bindings.end());
		return true;
	}
	return false;
}

void FInputSubsystem::ClearBindings()
{
	bindings.clear();
}

// -----------------------------------------------------------------
// Key State
// -----------------------------------------------------------------

void FInputSubsystem::SetKeyState(FKey Key, bool bPressed)
{
	keyStates[Key] = bPressed;
}

void FInputSubsystem::SetAxisValue(FKey Key, float Value)
{
	axisValues[Key] = Value;
}

bool FInputSubsystem::GetKeyState(FKey Key) const
{
	auto it = keyStates.find(Key);
	return it != keyStates.end() ? it->second : false;
}

// -----------------------------------------------------------------
// Query
// -----------------------------------------------------------------

const FInputActionInstance* FInputSubsystem::GetActionInstance(const FInputAction* action) const
{
	auto it = actionInstances.find(action);
	return it != actionInstances.end() ? &it->second : nullptr;
}

// -----------------------------------------------------------------
// Internal: rebuild mapping table
// -----------------------------------------------------------------

void FInputSubsystem::rebuildMappingTable()
{
	resolvedMappings.clear();

	for (const auto& ac : activeContexts)
	{
		for (const auto& mapping : ac.Context->GetMappings())
		{
			FResolvedMapping resolved;
			resolved.Action = mapping.Action;
			resolved.Key = mapping.Key;
			resolved.ContextPriority = ac.Priority;

			// Merge modifiers: mapping-level first, then action-level
			resolved.Modifiers = mapping.Modifiers;
			if (mapping.Action)
			{
				for (auto* mod : mapping.Action->GetModifiers())
				{
					resolved.Modifiers.push_back(mod);
				}
			}

			// Triggers: action-level overrides mapping-level
			if (mapping.Action && !mapping.Action->GetTriggers().empty())
			{
				resolved.Triggers = mapping.Action->GetTriggers();
			}
			else if (!mapping.Triggers.empty())
			{
				resolved.Triggers = mapping.Triggers;
			}
			// If no triggers at all, will use defaultTrigger in processInput

			resolvedMappings.push_back(std::move(resolved));
		}
	}
}

// -----------------------------------------------------------------
// Internal: per-frame processing pipeline
// -----------------------------------------------------------------

void FInputSubsystem::processInput(float deltaTime)
{
	// Reset action values for accumulation
	for (auto& [action, instance] : actionInstances)
	{
		instance.Value.Reset();
	}

	// Track consumed keys (for bConsumeInput)
	std::unordered_map<FKey, bool, FKey::Hash> consumedKeys;

	// Phase 1-3: For each resolved mapping, read key state, apply modifiers, accumulate
	for (const auto& mapping : resolvedMappings)
	{
		if (!mapping.Action)
		{
			continue;
		}

		// Check if key is consumed by higher-priority context
		if (mapping.Action->GetConsumeInput() && consumedKeys.count(mapping.Key))
		{
			continue;
		}

		// Read raw key state
		FInputActionValue rawValue;
		if (mapping.Key.IsAxisKey())
		{
			auto it = axisValues.find(mapping.Key);
			float axisVal = (it != axisValues.end()) ? it->second : 0.0f;
			rawValue = FInputActionValue(axisVal);
		}
		else
		{
			auto it = keyStates.find(mapping.Key);
			bool pressed = (it != keyStates.end()) ? it->second : false;
			rawValue = FInputActionValue(pressed);
		}

		// Promote to action's value type
		FVector v = rawValue.Get<FVector>();
		FInputActionValue typedValue(mapping.Action->GetValueType(), v);

		// Apply mapping-level modifiers (merged in rebuildMappingTable)
		for (auto* modifier : mapping.Modifiers)
		{
			typedValue = modifier->Modify(typedValue, deltaTime);
		}

		// Accumulate into action instance
		auto& instance = actionInstances[mapping.Action];
		instance.SourceAction = mapping.Action;

		if (mapping.Action->GetAccumulationBehavior() ==
			EInputActionAccumulationBehavior::Cumulative)
		{
			instance.Value += typedValue;
		}
		else // TakeHighestAbsoluteValue
		{
			FVector cur = instance.Value.Get<FVector>();
			FVector inc = typedValue.Get<FVector>();
			FVector result(
				FMath::Abs(inc.X) > FMath::Abs(cur.X) ? inc.X : cur.X,
				FMath::Abs(inc.Y) > FMath::Abs(cur.Y) ? inc.Y : cur.Y,
				FMath::Abs(inc.Z) > FMath::Abs(cur.Z) ? inc.Z : cur.Z);
			instance.Value = FInputActionValue(mapping.Action->GetValueType(), result);
		}

		// Mark key as consumed
		if (mapping.Action->GetConsumeInput() && typedValue.IsNonZero())
		{
			consumedKeys[mapping.Key] = true;
		}
	}

	// Phase 4-6: Evaluate triggers and fire bindings
	for (auto& [action, instance] : actionInstances)
	{
		// Sanitize NaN/Inf
		FVector v = instance.Value.Get<FVector>();
		if (std::isnan(v.X) || std::isinf(v.X) ||
			std::isnan(v.Y) || std::isinf(v.Y) ||
			std::isnan(v.Z) || std::isinf(v.Z))
		{
			ENIGMA_LOG(LogTemp, Warning, "[FInputSubsystem] NaN/Inf sanitized for action");
			instance.Value.Reset();
		}

		// Find triggers for this action from resolved mappings
		std::vector<IInputTrigger*> triggers;
		for (const auto& mapping : resolvedMappings)
		{
			if (mapping.Action == action && !mapping.Triggers.empty())
			{
				triggers = mapping.Triggers;
				break;
			}
		}

		// Use default Down trigger if none specified
		IInputTrigger* singleTrigger = nullptr;
		if (triggers.empty())
		{
			singleTrigger = &defaultTrigger;
			triggers.push_back(singleTrigger);
		}

		// Evaluate triggers (use first trigger's result)
		ETriggerState currentState = ETriggerState::None;
		for (auto* trigger : triggers)
		{
			currentState = trigger->UpdateState(instance.Value, deltaTime);
			break; // Use first trigger
		}

		// Determine event from state transition
		ETriggerEvent event = determineEvent(instance.LastTriggerState, currentState);

		// Update timing
		if (currentState != ETriggerState::None)
		{
			instance.ElapsedTime += deltaTime;
		}
		else
		{
			instance.ElapsedTime = 0.0f;
		}

		if (currentState == ETriggerState::Triggered)
		{
			instance.TriggeredTime += deltaTime;
		}
		else
		{
			instance.TriggeredTime = 0.0f;
		}

		instance.LastTriggerState = currentState;
		instance.LastTriggerEvent = event;

		// Fire matching bindings
		if (event != ETriggerEvent::None)
		{
			for (const auto& binding : bindings)
			{
				if (binding.Action == action &&
					(static_cast<uint8_t>(binding.Event) & static_cast<uint8_t>(event)) != 0)
				{
					binding.Callback.Execute(instance);
				}
			}
		}
	}

	// Reset axis values each frame (they are set fresh by FInputMessageBridge)
	axisValues.clear();
}

// -----------------------------------------------------------------
// Internal: state transition -> event mapping
// -----------------------------------------------------------------

ETriggerEvent FInputSubsystem::determineEvent(ETriggerState previous, ETriggerState current)
{
	if (previous == ETriggerState::None && current == ETriggerState::None)
	{
		return ETriggerEvent::None;
	}
	if (previous == ETriggerState::None && current == ETriggerState::Ongoing)
	{
		return ETriggerEvent::Started;
	}
	if (previous == ETriggerState::None && current == ETriggerState::Triggered)
	{
		return ETriggerEvent::Started | ETriggerEvent::Triggered;
	}
	if (previous == ETriggerState::Ongoing && current == ETriggerState::Ongoing)
	{
		return ETriggerEvent::Ongoing;
	}
	if (previous == ETriggerState::Ongoing && current == ETriggerState::Triggered)
	{
		return ETriggerEvent::Triggered;
	}
	if (previous == ETriggerState::Ongoing && current == ETriggerState::None)
	{
		return ETriggerEvent::Canceled;
	}
	if (previous == ETriggerState::Triggered && current == ETriggerState::Triggered)
	{
		return ETriggerEvent::Triggered;
	}
	if (previous == ETriggerState::Triggered && current == ETriggerState::Ongoing)
	{
		return ETriggerEvent::Ongoing;
	}
	if (previous == ETriggerState::Triggered && current == ETriggerState::None)
	{
		return ETriggerEvent::Completed;
	}
	return ETriggerEvent::None;
}

} // namespace Enigma
