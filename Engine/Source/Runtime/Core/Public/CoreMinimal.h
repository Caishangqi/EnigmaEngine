// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

// ---------------------------------------------------------------
// CoreMinimal.h -- Unified include header for the Core module.
//
// Any module that depends on Core can include this single header
// to get access to all essential Core types and utilities.
// This mirrors Unreal Engine's CoreMinimal.h pattern.
// ---------------------------------------------------------------

// Platform abstraction
#include "HAL/Platform.h"

// Module API export macro
#include "CoreAPI.generated.h"

// Logging system
#include "Logging/LogVerbosity.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"

// Assertion macros
#include "Misc/AssertionMacros.h"

// Module system
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleInitializerEntry.h"
#include "Modules/ModuleMacros.h"
#include "Modules/ModuleManager.h"
