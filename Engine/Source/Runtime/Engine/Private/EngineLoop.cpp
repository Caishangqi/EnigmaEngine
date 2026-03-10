// Copyright EnigmaEngine. All Rights Reserved.

#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "GenericPlatform/GenericApplication.h"
#include "Modules/ModuleManager.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ConfigDelegates.h"

#include "Logging/LogMacros.h"

#include <filesystem>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

namespace Enigma
{

DEFINE_LOG_CATEGORY_STATIC(LogInit, Info, All);

// ---------------------------------------------------------------
// RequestExit / IsExitRequested -- delegate to Core globals
// ---------------------------------------------------------------
void FEngineLoop::RequestExit()
{
    RequestEngineExit("FEngineLoop::RequestExit");
}

bool FEngineLoop::IsExitRequested() const
{
    return IsEngineExitRequested();
}

// ---------------------------------------------------------------
// Module phase registration
// ---------------------------------------------------------------
void FEngineLoop::AddModuleToPhase(ELoadingPhase phase, const std::string& moduleName)
{
    int idx = static_cast<int>(phase);
    if (idx >= 0 && idx < static_cast<int>(ELoadingPhase::None))
    {
        ModulesByPhase[idx].push_back(moduleName);
    }
}

void FEngineLoop::LoadModulesForPhase(ELoadingPhase phase)
{
    int idx = static_cast<int>(phase);
    if (idx < 0 || idx >= static_cast<int>(ELoadingPhase::None)) return;

    auto& modules = ModulesByPhase[idx];
    for (auto& name : modules)
    {
        ENIGMA_LOG(LogInit, Info, "Loading module '{}' (phase {})", name, idx);
        auto* mod = FModuleManager::Get().LoadModule(name);
        if (!mod)
        {
            ENIGMA_LOG(LogInit, Error, "Failed to load module '{}'", name);
        }
    }
}

// ---------------------------------------------------------------
// RegisterPluginModules -- add plugin DLL search paths and
//                          register modules for PostEngineInit
// ---------------------------------------------------------------
void FEngineLoop::RegisterPluginModules(
    const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& pluginDirs)
{
    auto& moduleMgr = FModuleManager::Get();

    for (const auto& [pluginName, pluginRoot, moduleNames] : pluginDirs)
    {
        // Add the plugin's Binaries/ directory as a DLL search path.
        // Convention: {PluginRoot}/Binaries/{Platform}/
        std::string binariesPath = pluginRoot;
        if (!binariesPath.empty() && binariesPath.back() != '/' && binariesPath.back() != '\\')
        {
            binariesPath += '/';
        }
#ifdef _WIN32
        binariesPath += "Binaries/Win64";
#elif defined(__APPLE__)
        binariesPath += "Binaries/Mac";
#else
        binariesPath += "Binaries/Linux";
#endif

        moduleMgr.AddDllSearchPath(binariesPath);

        ENIGMA_LOG(LogInit, Info, "Plugin '{}': added DLL search path '{}'",
            pluginName, binariesPath);

        // Register each module for PostEngineInit loading phase.
        for (const auto& moduleName : moduleNames)
        {
            AddModuleToPhase(ELoadingPhase::PostEngineInit, moduleName);

            ENIGMA_LOG(LogInit, Info, "Plugin '{}': registered module '{}' for PostEngineInit phase",
                pluginName, moduleName);
        }
    }
}

// ---------------------------------------------------------------
// PreInit -- load early-phase modules
// ---------------------------------------------------------------
int32_t FEngineLoop::PreInit(const char* cmdLine)
{
    ENIGMA_LOG(LogInit, Info, "PreInit begin (cmdLine: \"{}\")", cmdLine ? cmdLine : "");

    // Phase 0: EarliestPossible (Core, fundamental modules)
    LoadModulesForPhase(ELoadingPhase::EarliestPossible);

    // Create platform application (ApplicationCore)
    FGenericApplication::CreateApplication();

    // --- GConfig initialization ---
    // Resolve engine and project config directories from executable location.
    //
    // Walk-up discovery: starting from the exe directory, walk up the
    // directory tree looking for known marker files. This is robust
    // regardless of build directory depth (e.g. Intermediate/Build/
    // {Config}/Binaries/{CMakeConfig}/).
    //
    // Engine root marker:  {dir}/Engine/Config/BaseEngine.ini
    //                  or: {dir}/Config/BaseEngine.ini  (exe inside Engine/)
    // Project root marker: {dir}/Config/DefaultEngine.ini
    //
    // Shipped layout (StagedBuilds):
    //   {Root}/Engine/Config/BaseEngine.ini
    //   {Root}/{GameName}/Config/DefaultEngine.ini
    //   {Root}/{GameName}/Binaries/{Platform}/{GameExe}
    //
    // Development layout (build inside project):
    //   {EngineRoot}/Engine/Config/BaseEngine.ini
    //   {ProjectRoot}/Config/DefaultEngine.ini
    //   {ProjectRoot}/Intermediate/Build/{Config}/Binaries/{CMakeConfig}/{GameExe}
    std::string engineConfigDir;
    std::string projectConfigDir;
    {
#ifdef _WIN32
        char exePath[MAX_PATH] = {};
        if (::GetModuleFileNameA(nullptr, exePath, MAX_PATH) > 0)
        {
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

            // Set TargetName for .modules manifest lookup.
            // TargetName = exe filename without extension (e.g. "EnigmaEngine").
            {
                std::string exeStem = std::filesystem::path(exePath).stem().string();
                FModuleManager::Get().SetTargetName(exeStem);
                ENIGMA_LOG(LogInit, Info, "TargetName set to '{}'", exeStem);
            }

            // Walk up from exe directory to find engine and project config.
            // Stop after a reasonable depth (16 levels) to avoid infinite loops.
            //
            // Additionally handles the Shipped root-launcher case where the exe
            // sits at the package root and the project config is in a sibling
            // subdirectory: {Root}/{GameName}/Config/DefaultEngine.ini.
            // The game name is extracted from Engine/Config/StagedBuild_*.ini.
            std::filesystem::path current = exeDir;
            for (int depth = 0; depth < 16; ++depth)
            {
                // Engine config: {current}/Engine/Config/BaseEngine.ini
                // (exe is somewhere inside or alongside the engine tree)
                if (engineConfigDir.empty())
                {
                    std::filesystem::path candidate = current / "Engine" / "Config" / "BaseEngine.ini";
                    if (std::filesystem::exists(candidate))
                    {
                        engineConfigDir = std::filesystem::canonical(candidate.parent_path()).string();

                        // Shipped layout: project config is a sibling of Engine/.
                        // Parse StagedBuild_*.ini to discover the game name, then
                        // resolve {current}/{GameName}/Config/.
                        if (projectConfigDir.empty())
                        {
                            std::filesystem::path engineCfgDir = candidate.parent_path();
                            for (const auto& entry : std::filesystem::directory_iterator(engineCfgDir))
                            {
                                std::string fname = entry.path().filename().string();
                                if (fname.starts_with("StagedBuild_") && fname.ends_with(".ini"))
                                {
                                    // Extract game name: StagedBuild_{GameName}.ini
                                    std::string gameName = fname.substr(12, fname.size() - 12 - 4);
                                    std::filesystem::path projCfg = current / gameName / "Config" / "DefaultEngine.ini";
                                    if (std::filesystem::exists(projCfg))
                                    {
                                        projectConfigDir = std::filesystem::canonical(projCfg.parent_path()).string();
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }

                // Engine config: {current}/Config/BaseEngine.ini
                // (exe is inside the Engine directory itself)
                if (engineConfigDir.empty())
                {
                    std::filesystem::path candidate = current / "Config" / "BaseEngine.ini";
                    if (std::filesystem::exists(candidate))
                    {
                        engineConfigDir = std::filesystem::canonical(candidate.parent_path()).string();
                    }
                }

                // Project config: {current}/Config/DefaultEngine.ini
                // (exe is inside the project directory tree)
                if (projectConfigDir.empty())
                {
                    std::filesystem::path candidate = current / "Config" / "DefaultEngine.ini";
                    if (std::filesystem::exists(candidate))
                    {
                        projectConfigDir = std::filesystem::canonical(candidate.parent_path()).string();
                    }
                }

                // Stop early if both found
                if (!engineConfigDir.empty() && !projectConfigDir.empty())
                {
                    break;
                }

                // Move up one directory
                std::filesystem::path parent = current.parent_path();
                if (parent == current)
                {
                    break; // Reached filesystem root
                }
                current = parent;
            }
        }
#endif
        // Command-line override: --engine-dir=<path>
        if (cmdLine != nullptr)
        {
            std::string cmdStr(cmdLine);
            const std::string engineFlag = "--engine-dir=";
            auto pos = cmdStr.find(engineFlag);
            if (pos != std::string::npos)
            {
                auto start = pos + engineFlag.size();
                auto end = cmdStr.find(' ', start);
                std::string dir = cmdStr.substr(start, end - start);
                std::filesystem::path configPath = std::filesystem::path(dir) / "Config";
                if (std::filesystem::exists(configPath))
                {
                    engineConfigDir = std::filesystem::canonical(configPath).string();
                }
            }

            // Command-line override: --project-dir=<path>
            const std::string projectFlag = "--project-dir=";
            pos = cmdStr.find(projectFlag);
            if (pos != std::string::npos)
            {
                auto start = pos + projectFlag.size();
                auto end = cmdStr.find(' ', start);
                std::string dir = cmdStr.substr(start, end - start);
                std::filesystem::path configPath = std::filesystem::path(dir) / "Config";
                if (std::filesystem::exists(configPath))
                {
                    projectConfigDir = std::filesystem::canonical(configPath).string();
                }

                // Register game DLL search path: {ProjectDir}/Binaries/{Platform}/
#ifdef _WIN32
                std::filesystem::path gameBin = std::filesystem::path(dir) / "Binaries" / "Win64";
                if (std::filesystem::exists(gameBin))
                {
                    std::string gameBinStr = std::filesystem::canonical(gameBin).string();
                    FModuleManager::Get().AddDllSearchPath(gameBinStr);
                    ENIGMA_LOG(LogInit, Info, "Game DLL search path: {}", gameBinStr);
                }

                // Register plugin DLL search paths: {ProjectDir}/Plugins/*/Binaries/{Platform}/
                std::filesystem::path pluginsDir = std::filesystem::path(dir) / "Plugins";
                if (std::filesystem::exists(pluginsDir) && std::filesystem::is_directory(pluginsDir))
                {
                    for (const auto& entry : std::filesystem::directory_iterator(pluginsDir))
                    {
                        if (!entry.is_directory()) continue;
                        std::filesystem::path pluginBin = entry.path() / "Binaries" / "Win64";
                        if (std::filesystem::exists(pluginBin))
                        {
                            std::string pluginBinStr = std::filesystem::canonical(pluginBin).string();
                            FModuleManager::Get().AddDllSearchPath(pluginBinStr);
                            ENIGMA_LOG(LogInit, Info, "Plugin DLL search path: {}", pluginBinStr);
                        }
                    }
                }
#endif
            }
        }

        // --- Engine plugin DLL search paths ---
        // Derive engine root from engineConfigDir: {EngineRoot}/Engine/Config -> {EngineRoot}/Engine
        // Then scan {EngineRoot}/Engine/Plugins/*/Binaries/{Platform}/
        if (!engineConfigDir.empty())
        {
#ifdef _WIN32
            std::filesystem::path enginePluginsDir =
                std::filesystem::path(engineConfigDir).parent_path() / "Plugins";
            if (std::filesystem::exists(enginePluginsDir) && std::filesystem::is_directory(enginePluginsDir))
            {
                for (const auto& entry : std::filesystem::directory_iterator(enginePluginsDir))
                {
                    if (!entry.is_directory()) continue;
                    std::filesystem::path pluginBin = entry.path() / "Binaries" / "Win64";
                    if (std::filesystem::exists(pluginBin))
                    {
                        std::string pluginBinStr = std::filesystem::canonical(pluginBin).string();
                        FModuleManager::Get().AddDllSearchPath(pluginBinStr);
                        ENIGMA_LOG(LogInit, Info, "Engine plugin DLL search path: {}",
                            pluginBinStr);
                    }
                }
            }
#endif
        }
    }

    GConfig = new FConfigCacheIni();
    GConfig->Initialize(engineConfigDir, projectConfigDir);
    FConfigDelegates::OnConfigReadyForUse.Broadcast();
    ENIGMA_LOG(LogInit, Info, "GConfig initialized (engine: \"{}\", project: \"{}\")",
        engineConfigDir, projectConfigDir);

    // Phase 1: PostConfigInit (config-dependent modules -- can now read GConfig)
    LoadModulesForPhase(ELoadingPhase::PostConfigInit);

    // Phase 2: PreLoadingScreen (Engine, Renderer)
    LoadModulesForPhase(ELoadingPhase::PreLoadingScreen);

    ENIGMA_LOG(LogInit, Info, "PreInit complete");
    return 0;
}

// ---------------------------------------------------------------
// Init -- create GEngine, load remaining modules, start
// ---------------------------------------------------------------
int32_t FEngineLoop::Init()
{
    ENIGMA_LOG(LogInit, Info, "Init begin");

    // Create GEngine (FGameEngine for game mode)
    auto* gameEngine = new FGameEngine();
    GEngine = gameEngine;

    // Phase 3: Default (game modules) -- loaded before engine Init
    // so game modules can register factories (e.g. GameInstance factory)
    LoadModulesForPhase(ELoadingPhase::Default);

    // Auto-discover and load all module DLLs from the executable directory
    // and registered DLL search paths (game/plugin directories).
    //
    // Two-phase approach: scan ALL directories for DLLs first, then
    // initialize modules. This ensures cross-directory DLL dependencies
    // are resolved (e.g. game module depends on plugin DLL).
    {
        std::string binDir;
#ifdef _WIN32
        char exePath[MAX_PATH] = {};
        if (::GetModuleFileNameA(nullptr, exePath, MAX_PATH) > 0)
        {
            binDir = exePath;
            auto pos = binDir.find_last_of("\\/");
            if (pos != std::string::npos)
                binDir = binDir.substr(0, pos);
        }
#endif
        if (!binDir.empty())
        {
            // Phase 1: Scan all directories -- load DLLs into process
            //          (triggers static FModuleInitializerEntry registration)
            FModuleManager::Get().ScanDllsFromDirectory(binDir);

            for (const auto& searchPath : FModuleManager::Get().GetDllSearchPaths())
            {
                if (searchPath != binDir)
                {
                    FModuleManager::Get().ScanDllsFromDirectory(searchPath);
                }
            }

            // Phase 2: Initialize all newly registered modules
            FModuleManager::Get().LoadAllRegisteredModules();
        }
        else
        {
            // Fallback: just initialize already-registered modules
            FModuleManager::Get().LoadAllRegisteredModules();
        }
    }

    // Initialize engine (uses registered factories, e.g. CreateGameInstance)
    GEngine->Init(this);

    // Phase 4: PostEngineInit (plugins, late modules)
    // Loaded AFTER GEngine->Init() so modules can access fully initialized engine.
    LoadModulesForPhase(ELoadingPhase::PostEngineInit);

    // Start engine (initializes GameInstance, begins game loop)
    GEngine->Start();

    // Initialize timing
    LastTickTime = Clock::now();
    FrameNumber  = 0;
    bIsRunning   = true;

    ENIGMA_LOG(LogInit, Info, "Init complete -- engine running");
    return 0;
}

// ---------------------------------------------------------------
// Tick -- one frame
// ---------------------------------------------------------------
void FEngineLoop::Tick()
{
    if (!bIsRunning || !GEngine) return;

    // Frame rate control (REQ-5)
    GEngine->UpdateTimeAndHandleMaxTickRate();

    // Pump OS messages before game tick
    if (auto* app = FGenericApplication::GetApplication())
    {
        app->PumpMessages(DeltaTime);
        app->ProcessDeferredEvents(DeltaTime);
    }

    // Calculate DeltaTime
    auto now = Clock::now();
    std::chrono::duration<float> elapsed = now - LastTickTime;
    DeltaTime    = elapsed.count();
    LastTickTime = now;

    // Engine tick
    GEngine->Tick(DeltaTime);

    ++FrameNumber;
}

// ---------------------------------------------------------------
// Exit -- shutdown and cleanup
// ---------------------------------------------------------------
void FEngineLoop::Exit()
{
    ENIGMA_LOG(LogInit, Info, "Exit begin");

    bIsRunning = false;

    // Shutdown and destroy GEngine
    if (GEngine)
    {
        GEngine->Shutdown();
        delete GEngine;
        GEngine = nullptr;
    }

    // Destroy platform application (and all windows)
    if (auto* app = FGenericApplication::GetApplication())
    {
        delete app;
    }

    // Unload all modules in reverse load-order
    FModuleManager::Get().UnloadAllModules();

    // Destroy GConfig last (after module unload, matches UE ordering)
    if (GConfig)
    {
        delete GConfig;
        GConfig = nullptr;
    }

    ENIGMA_LOG(LogInit, Info, "Exit complete");
}

} // namespace Enigma
