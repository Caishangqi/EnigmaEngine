// Copyright EnigmaEngine. All Rights Reserved.

/// @file EnhancedInputModule.cpp
/// @brief EnhancedInput plugin module implementation.

#include "EnhancedInputModule.h"
#include "InputSubsystem.h"
#include "Modules/ModuleMacros.h"
#include "Engine/Engine.h"
#include "Subsystems/SubsystemCollection.h"

#include <cstdio>

namespace Enigma
{

void FEnhancedInputModule::StartupModule()
{
	std::printf("[EnhancedInput] StartupModule\n");

	// Register FInputSubsystem with the engine's SubsystemCollection.
	if (GEngine)
	{
		GEngine->GetSubsystemCollection().RegisterSubsystem<FInputSubsystem>();
	}
}

void FEnhancedInputModule::ShutdownModule()
{
	std::printf("[EnhancedInput] ShutdownModule\n");
}

} // namespace Enigma

IMPLEMENT_MODULE(Enigma::FEnhancedInputModule, EnhancedInput)
