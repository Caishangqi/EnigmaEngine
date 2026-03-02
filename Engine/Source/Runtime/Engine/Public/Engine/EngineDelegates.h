// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EngineAPI.generated.h"
#include "Delegates/MulticastDelegate.h"

// -------------------------------------------------------------
// EngineDelegates.h
//
// Engine-level delegate events for window and rendering lifecycle.
// Mirrors UE's engine delegates (simplified subset).
// -------------------------------------------------------------

namespace Enigma
{

class FGenericWindow;

/// Engine-level delegate events for window and rendering lifecycle.
struct ENGINE_API FEngineDelegates
{
    /// Broadcast after the game window is created in FGameEngine::Init().
    /// Params: FGenericWindow* window
    using FOnGameWindowCreated = TMulticastDelegate<FGenericWindow* /*window*/>;
    static FOnGameWindowCreated OnGameWindowCreated;

    /// Broadcast before the game window is destroyed in FGameEngine::Shutdown().
    /// Params: FGenericWindow* window
    using FOnGameWindowDestroyed = TMulticastDelegate<FGenericWindow* /*window*/>;
    static FOnGameWindowDestroyed OnGameWindowDestroyed;

    /// Broadcast before each render frame.
    using FOnPreRender = TMulticastDelegate<>;
    static FOnPreRender OnPreRender;

    /// Broadcast after each render frame.
    using FOnPostRender = TMulticastDelegate<>;
    static FOnPostRender OnPostRender;
};

} // namespace Enigma
