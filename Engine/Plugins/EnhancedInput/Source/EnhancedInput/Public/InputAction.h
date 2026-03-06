// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputAction.h
/// @brief Abstract input action definition.

#include "InputKeys.h"  // for ENHANCEDINPUT_API
#include "InputActionValue.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Enigma
{

class IInputTrigger;
class IInputModifier;

/// @brief How values from multiple mappings to the same action are merged.
enum class EInputActionAccumulationBehavior : uint8_t
{
	TakeHighestAbsoluteValue,
	Cumulative
};

/// @brief Abstract input action definition.
///
/// UE equivalent: UInputAction (EnhancedInput/Public/InputAction.h)
/// UE's UInputAction inherits UDataAsset; ours is a plain C++ class.
class ENHANCEDINPUT_API FInputAction
{
public:
	explicit FInputAction(
		std::string name,
		EInputActionValueType valueType = EInputActionValueType::Boolean);

	const std::string& GetName() const;
	EInputActionValueType GetValueType() const;
	bool GetConsumeInput() const;
	EInputActionAccumulationBehavior GetAccumulationBehavior() const;
	const std::vector<IInputTrigger*>& GetTriggers() const;
	const std::vector<IInputModifier*>& GetModifiers() const;

	void SetConsumeInput(bool bConsume);
	void SetAccumulationBehavior(EInputActionAccumulationBehavior behavior);
	void AddTrigger(IInputTrigger* trigger);
	void AddModifier(IInputModifier* modifier);

private:
	std::string Name;
	EInputActionValueType ValueType;
	bool bConsumeInput = true;
	EInputActionAccumulationBehavior AccumulationBehavior =
		EInputActionAccumulationBehavior::TakeHighestAbsoluteValue;
	std::vector<IInputTrigger*> Triggers;   // action-level (non-owning)
	std::vector<IInputModifier*> Modifiers; // action-level (non-owning)
};

} // namespace Enigma
