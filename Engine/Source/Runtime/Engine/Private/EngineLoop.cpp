// Copyright EnigmaEngine. All Rights Reserved.

#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "GenericPlatform/GenericApplication.h"
#include "Modules/ModuleManager.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ConfigDelegates.h"

#include <cstdio>
#include <filesystem>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

namespace Enigma
{

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
        std::printf("[FEngineLoop] Loading module '%s' (phase %d)\n",
            name.c_str(), idx);
        auto* mod = FModuleManager::Get().LoadModule(name);
        if (!mod)
        {
            std::fprintf(stderr,
                "[FEngineLoop] ERROR: Failed to load module '%s'\n",
                name.c_str());
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

        std::printf("[FEngineLoop] Plugin '%s': added DLL search path '%s'\n",
            pluginName.c_str(), binariesPath.c_str());

        // Register each module for PostEngineInit loading phase.
        for (const auto& moduleName : moduleNames)
        {
            AddModuleToPhase(ELoadingPhase::PostEngineInit, moduleName);

            std::printf("[FEngineLoop] Plugin '%s': registered module '%s' "
                "for PostEngineInit phase\n",
                pluginName.c_str(), moduleName.c_str());
        }
    }
}

// ---------------------------------------------------------------
// PreInit -- load early-phase modules
// ---------------------------------------------------------------
int32_t FEngineLoop::PreInit(const char* cmdLine)
{
    std::printf("[FEngineLoop] PreInit begin (cmdLine: \"%s\")\n",
        cmdLine ? cmdLine : "");

    // Phase 0: EarliestPossible (Core, fundamental modules)
    LoadModulesForPhase(ELoadingPhase::EarliestPossible);

    // Create platform application (ApplicationCore)
    FGenericApplication::CreateApplication();

    // --- GConfig initialization ---
    // Resolve engine and project config directories from executable location.
    // Mirrors UE's FPaths approach: fixed relative paths from exe.
    //
    // Modular (Development/DebugGame/Debug):
    //   EXE at {Engine}/Binaries/{Platform}/
    //   Engine config: ../../Config
    //   Project config: resolved via --project-dir= or ../../../{ProjectName}/Config
    //
    // Shipped (Shipping):
    //   EXE at {Project}/Binaries/{Platform}/
    //   Engine config: ../../../Engine/Config
    //   Project config: ../../Config
    std::string engineConfigDir;
    std::string projectConfigDir;
    {
#ifdef _WIN32
        char exePath[MAX_PATH] = {};
        if (::GetModuleFileNameA(nullptr, exePath, MAX_PATH) > 0)
        {
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

            // Try Modular layout first: EXE in {Engine}/Binaries/{Platform}/
            // Engine config: ../../Config
            std::filesystem::path engineCfgModular = exeDir / ".." / ".." / "Config";
            if (std::filesystem::exists(engineCfgModular))
            {
                engineConfigDir = std::filesystem::canonical(engineCfgModular).string();
            }

            // Fallback: Shipped layout: EXE in {Project}/Binaries/{Platform}/
            // Engine config: ../../../Engine/Config
            if (engineConfigDir.empty())
            {
                std::filesystem::path engineCfgShipped = exeDir / ".." / ".." / ".." / "Engine" / "Config";
                if (std::filesystem::exists(engineCfgShipped))
                {
                    engineConfigDir = std::filesystem::canonical(engineCfgShipped).string();
                }
            }

            // Project config: try Shipped layout first (../../Config)
            std::filesystem::path projCfgShipped = exeDir / ".." / ".." / "Config";
            if (std::filesystem::exists(projCfgShipped))
            {
                projectConfigDir = std::filesystem::canonical(projCfgShipped).string();
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
                    std::printf("[FEngineLoop] Game DLL search path: %s\n", gameBinStr.c_str());
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
                            std::printf("[FEngineLoop] Plugin DLL search path: %s\n", pluginBinStr.c_str());
                        }
                    }
                }
#endif
            }
        }
    }

    GConfig = new FConfigCacheIni();
    GConfig->Initialize(engineConfigDir, projectConfigDir);
    FConfigDelegates::OnConfigReadyForUse.Broadcast();
    std::printf("[FEngineLoop] GConfig initialized (engine: \"%s\", project: \"%s\")\n",
        engineConfigDir.c_str(), projectConfigDir.c_str());

    // Phase 1: PostConfigInit (config-dependent modules -- can now read GConfig)
    LoadModulesForPhase(ELoadingPhase::PostConfigInit);

    // Phase 2: PreLoadingScreen (Engine, Renderer)
    LoadModulesForPhase(ELoadingPhase::PreLoadingScreen);

    std::printf("[FEngineLoop] PreInit complete\n");
    return 0;
}

// ---------------------------------------------------------------
// Init -- create GEngine, load remaining modules, start
// ---------------------------------------------------------------
int32_t FEngineLoop::Init()
{
    std::printf("[FEngineLoop] Init begin\n");

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

    std::printf("[FEngineLoop] Init complete -- engine running\n");
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
    std::printf("[FEngineLoop] Exit begin\n");

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

    std::printf("[FEngineLoop] Exit complete\n");
}

} // namespace Enigma
