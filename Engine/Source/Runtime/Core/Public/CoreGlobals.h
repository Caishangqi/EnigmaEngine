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

} // namespace Enigma
