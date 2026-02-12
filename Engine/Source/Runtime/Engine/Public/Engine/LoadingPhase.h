// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include <cstdint>

namespace Enigma
{

// ---------------------------------------------------------------
// ELoadingPhase -- module loading phase ordering (REQ-004)
//
// Modules are loaded in phase order during engine startup.
// FEngineLoop::PreInit loads phases 0-2, Init loads phases 3-4.
// Mirrors Unreal Engine's ELoadingPhase.
// ---------------------------------------------------------------
enum class ELoadingPhase : uint8_t
{
    EarliestPossible,   // Core modules, immediately after file system
    PostConfigInit,     // After config system init (e.g. Json)
    PreLoadingScreen,   // Before loading screen (Engine, Renderer)
    Default,            // Default phase (game modules)
    PostEngineInit,     // After GEngine created (plugins, late modules)
    None                // Not auto-loaded (manual only)
};

} // namespace Enigma
