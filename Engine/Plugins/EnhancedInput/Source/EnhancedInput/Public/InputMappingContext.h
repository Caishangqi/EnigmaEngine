// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputMappingContext.h
/// @brief Key-to-action mapping context with runtime activation.

#include "InputKeys.h"  // for ENHANCEDINPUT_API, FKey
#include "InputModifiers.h"
#include "InputTriggers.h"

#include <string>
#include <vector>

namespace Enigma
{

class FInputAction;

/// @brief A single key-to-action mapping with optional per-mapping modifiers/triggers.
struct FActionKeyMapping
{
	const FInputAction* Action = nullptr;
	FKey Key;
	std::vector<IInputModifier*> Modifiers; // mapping-level (non-owning)
	std::vector<IInputTrigger*> Triggers;   // mapping-level (non-owning)
};

/// @brief A collection of key-to-action mappings that can be activated/deactivated at runtime.
class ENHANCEDINPUT_API FInputMappingContext
{
public:
	explicit FInputMappingContext(std::string name);

	const std::string& GetName() const;

	/// Add a mapping. Returns reference for chaining modifier/trigger setup.
	FActionKeyMapping& MapKey(const FInputAction* Action, FKey Key);

	/// Remove all mappings for a specific action + key combination.
	void UnmapKey(const FInputAction* Action, FKey Key);

	/// Remove all mappings for an action.
	void UnmapAction(const FInputAction* action);

	/// Get all mappings (read-only).
	const std::vector<FActionKeyMapping>& GetMappings() const;

private:
	std::string Name;
	std::vector<FActionKeyMapping> Mappings;
};

} // namespace Enigma
