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

// Containers
#include "Containers/Array.h"
#include "Containers/ArrayView.h"

// Delegate system (forward declarations)
#include "Delegates/DelegateFwd.h"

// Module system
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleInitializerEntry.h"
#include "Modules/ModuleMacros.h"
#include "Modules/ModuleManager.h"

// Math (ordered by dependency layer)
#include "Math/MathFwd.h"
#include "Math/MathUtility.h"
#include "Math/Vector2D.h"
#include "Math/Color.h"
#include "Math/LinearColor.h"
#include "Math/Vector.h"
#include "Math/IntVector.h"
#include "Math/Vector4.h"
#include "Math/Quat.h"
#include "Math/Matrix.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
