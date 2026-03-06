// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputMappingContext.cpp
/// @brief Implementation of FInputMappingContext.

#include "InputMappingContext.h"
#include "InputAction.h"

#include <algorithm>

namespace Enigma
{

FInputMappingContext::FInputMappingContext(std::string name)
	: Name(std::move(name))
{
}

const std::string& FInputMappingContext::GetName() const
{
	return Name;
}

FActionKeyMapping& FInputMappingContext::MapKey(const FInputAction* Action, FKey Key)
{
	Mappings.push_back({Action, std::move(Key), {}, {}});
	return Mappings.back();
}

void FInputMappingContext::UnmapKey(const FInputAction* Action, FKey Key)
{
	Mappings.erase(
		std::remove_if(Mappings.begin(), Mappings.end(),
			[&](const FActionKeyMapping& m)
			{
				return m.Action == Action && m.Key == Key;
			}),
		Mappings.end());
}

void FInputMappingContext::UnmapAction(const FInputAction* action)
{
	Mappings.erase(
		std::remove_if(Mappings.begin(), Mappings.end(),
			[&](const FActionKeyMapping& m)
			{
				return m.Action == action;
			}),
		Mappings.end());
}

const std::vector<FActionKeyMapping>& FInputMappingContext::GetMappings() const
{
	return Mappings;
}

} // namespace Enigma
