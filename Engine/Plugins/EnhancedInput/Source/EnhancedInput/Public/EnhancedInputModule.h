// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file EnhancedInputModule.h
/// @brief EnhancedInput plugin module interface.

#include "Modules/ModuleInterface.h"

namespace Enigma
{

/// @brief EnhancedInput plugin module.
///
/// Registers FInputSubsystem with the engine's SubsystemCollection
/// during StartupModule.
class FEnhancedInputModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
};

} // namespace Enigma
