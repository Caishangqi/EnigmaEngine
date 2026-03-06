// Copyright EnigmaEngine. All Rights Reserved.

/// @file TestStubs.cpp
/// @brief Minimal stubs for Engine/ApplicationCore symbols needed by EnhancedInput.
/// Avoids pulling in platform-specific code for unit tests.

#include "Engine/Engine.h"
#include "GenericPlatform/GenericApplication.h"

namespace Enigma
{

// Engine global pointer
FEngine* GEngine = nullptr;

// ApplicationCore stubs — no real application in test environment.
FGenericApplication* FGenericApplication::s_application = nullptr;

FGenericApplication* FGenericApplication::GetApplication()
{
	return nullptr;
}

void FGenericApplication::SetMessageHandler(FGenericApplicationMessageHandler* handler)
{
	MessageHandler = handler;
}

FGenericApplicationMessageHandler* FGenericApplication::GetMessageHandler() const
{
	return MessageHandler;
}

} // namespace Enigma
