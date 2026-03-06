// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputAction.cpp
/// @brief Implementation of FInputAction.

#include "InputAction.h"

namespace Enigma
{

FInputAction::FInputAction(std::string name, EInputActionValueType valueType)
	: Name(std::move(name))
	, ValueType(valueType)
{
}

const std::string& FInputAction::GetName() const
{
	return Name;
}

EInputActionValueType FInputAction::GetValueType() const
{
	return ValueType;
}

bool FInputAction::GetConsumeInput() const
{
	return bConsumeInput;
}

EInputActionAccumulationBehavior FInputAction::GetAccumulationBehavior() const
{
	return AccumulationBehavior;
}

const std::vector<IInputTrigger*>& FInputAction::GetTriggers() const
{
	return Triggers;
}

const std::vector<IInputModifier*>& FInputAction::GetModifiers() const
{
	return Modifiers;
}

void FInputAction::SetConsumeInput(bool bConsume)
{
	bConsumeInput = bConsume;
}

void FInputAction::SetAccumulationBehavior(EInputActionAccumulationBehavior behavior)
{
	AccumulationBehavior = behavior;
}

void FInputAction::AddTrigger(IInputTrigger* trigger)
{
	Triggers.push_back(trigger);
}

void FInputAction::AddModifier(IInputModifier* modifier)
{
	Modifiers.push_back(modifier);
}

} // namespace Enigma
