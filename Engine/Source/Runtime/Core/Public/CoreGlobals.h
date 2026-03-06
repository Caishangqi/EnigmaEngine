// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"

// -------------------------------------------------------------
// CoreGlobals.h
//
// Global pointers for the Core module.
// Follows UE's CoreGlobals.h pattern: header in Public/ root,
// implementation in Private/Misc/CoreGlobals.cpp.
//
// Modules that need config explicitly include this header.
// CoreMinimal.h provides forward declarations only.
// -------------------------------------------------------------

namespace Enigma
{

class FConfigCacheIni;

/// Global config cache pointer.
/// Initialized by FEngineLoop::PreInit(), destroyed by FEngineLoop::Exit().
/// UE equivalent: extern CORE_API FConfigCacheIni* GConfig;
extern CORE_API FConfigCacheIni* GConfig;

// -----------------------------------------------------------------
// Engine exit request (UE equivalent: CoreGlobals.h)
//
// Any module can call RequestEngineExit() to signal the engine loop
// to shut down. IsEngineExitRequested() is checked each frame by
// FEngineLoop::Tick().
// -----------------------------------------------------------------

/// Request that the engine exit as soon as it can safely do so.
/// @param reasonString  Human-readable reason for the exit request.
CORE_API void RequestEngineExit(const char* reasonString);

/// Check if an engine exit has been requested.
CORE_API bool IsEngineExitRequested();

} // namespace Enigma
