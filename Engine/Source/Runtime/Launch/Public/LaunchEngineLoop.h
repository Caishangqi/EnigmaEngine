// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "LaunchAPI.generated.h"
#include "EngineLoop.h"

namespace Enigma
{

// ---------------------------------------------------------------
// GEngineLoop -- the single global FEngineLoop instance (REQ-020)
//
// Owned by the Launch module. GuardedMain drives its lifecycle.
// Other systems may query GEngineLoop for frame info, exit
// requests, etc.
// ---------------------------------------------------------------
extern LAUNCH_API FEngineLoop GEngineLoop;

} // namespace Enigma
