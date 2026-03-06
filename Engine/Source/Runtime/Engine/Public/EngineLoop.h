// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EngineAPI.generated.h"
#include "Engine/LoadingPhase.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace Enigma
{

// ---------------------------------------------------------------
// FEngineLoop -- drives the engine lifecycle (REQ-004, REQ-019)
//
// Mirrors Unreal Engine's FEngineLoop:
//   PreInit  -> load EarliestPossible / PostConfigInit / PreLoadingScreen modules
//   Init     -> create GEngine, load Default / PostEngineInit modules, Start
//   Tick     -> pump messages, compute DeltaTime, GEngine->Tick
//   Exit     -> GEngine->Shutdown, unload all modules
//
// Module loading is driven by a phase-ordered list populated
// externally (e.g. by project configuration or BuildTool).
// ---------------------------------------------------------------
class ENGINE_API FEngineLoop
{
public:
    FEngineLoop() = default;
    ~FEngineLoop() = default;

    // Non-copyable
    FEngineLoop(const FEngineLoop&) = delete;
    FEngineLoop& operator=(const FEngineLoop&) = delete;

    // ----- Lifecycle -----

    /// Phase 1: Platform init, load early modules.
    /// Returns 0 on success, non-zero on failure.
    int32_t PreInit(const char* cmdLine);

    /// Phase 2: Create GEngine, load default/post modules, start engine.
    /// Returns 0 on success, non-zero on failure.
    int32_t Init();

    /// Phase 3: One frame -- message pump, DeltaTime, GEngine->Tick.
    void Tick();

    /// Phase 4: Shutdown GEngine, unload all modules.
    void Exit();

    // ----- Module phase registration -----

    /// Register a module to be loaded during a specific phase.
    void AddModuleToPhase(ELoadingPhase phase, const std::string& moduleName);

    /// Register plugin modules for loading during PostEngineInit phase.
    /// Scans the given plugin directories, adds their Binaries/ paths
    /// to FModuleManager's DLL search paths, and registers each plugin
    /// module for the PostEngineInit loading phase.
    ///
    /// @param pluginDirs  List of {PluginName, PluginRootDir, ModuleNames} tuples.
    void RegisterPluginModules(
        const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& pluginDirs);

    // ----- Queries -----

    bool IsRunning() const { return bIsRunning; }
    float GetDeltaTime() const { return DeltaTime; }
    int64_t GetFrameNumber() const { return FrameNumber; }

    /// Request engine exit (delegates to Core's RequestEngineExit).
    void RequestExit();
    bool IsExitRequested() const;

private:
    /// Load all modules registered for the given phase.
    void LoadModulesForPhase(ELoadingPhase phase);

    // Per-phase module lists
    std::vector<std::string> ModulesByPhase[static_cast<int>(ELoadingPhase::None)];

    // Timing
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point LastTickTime{};
    float             DeltaTime    = 0.0f;
    int64_t           FrameNumber  = 0;

    // State
    bool bIsRunning     = false;
};

} // namespace Enigma
