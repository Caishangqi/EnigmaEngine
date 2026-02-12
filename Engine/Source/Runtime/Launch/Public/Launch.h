// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "LaunchAPI.generated.h"

#include <cstdint>

namespace Enigma
{

// ---------------------------------------------------------------
// GuardedMain -- top-level engine entry point (REQ-020)
//
// Orchestrates the full FEngineLoop lifecycle:
//   1. PreInit  -- platform init, early module loading
//   2. Init     -- create GEngine, load remaining modules
//   3. Tick     -- main loop until exit is requested
//   4. Exit     -- shutdown engine, unload modules
//
// Called by platform-specific entry points (main / WinMain).
// Returns 0 on success, non-zero on failure.
// ---------------------------------------------------------------
LAUNCH_API int32_t GuardedMain(const char* cmdLine);

// ---------------------------------------------------------------
// IsEngineExitRequested -- global exit query
//
// Convenience function that checks GEngineLoop.IsExitRequested().
// Can be called from anywhere to test if shutdown was requested.
// ---------------------------------------------------------------
LAUNCH_API bool IsEngineExitRequested();

} // namespace Enigma
