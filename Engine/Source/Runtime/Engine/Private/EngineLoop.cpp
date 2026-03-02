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
    // Both Development and Shipped layouts place the exe at:
    //   {Project}/Binaries/{Platform}/
    //
    // Engine config: ../../../Engine/Config  (3 levels up to repo root, then Engine/Config)
    // Project config: ../../Config           (2 levels up to project root, then Config)
    std::string engineConfigDir;
    std::string projectConfigDir;
    {
#ifdef _WIN32
        char exePath[MAX_PATH] = {};
        if (::GetModuleFileNameA(nullptr, exePath, MAX_PATH) > 0)
        {
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

            // Engine config: ../../../Engine/Config
            std::filesystem::path engineCfg = exeDir / ".." / ".." / ".." / "Engine" / "Config";
            if (std::filesystem::exists(engineCfg))
            {
                engineConfigDir = std::filesystem::canonical(engineCfg).string();
            }

            // Project config: ../../Config
            std::filesystem::path projCfg = exeDir / ".." / ".." / "Config";
            if (std::filesystem::exists(projCfg))
            {
                projectConfigDir = std::filesystem::canonical(projCfg).string();
            }
        }
#endif
        // Command-line override: --engine-dir=<path>
        if (cmdLine != nullptr)
        {
            std::string cmdStr(cmdLine);
            const std::string flag = "--engine-dir=";
            auto pos = cmdStr.find(flag);
            if (pos != std::string::npos)
            {
                auto start = pos + flag.size();
                auto end = cmdStr.find(' ', start);
                std::string dir = cmdStr.substr(start, end - start);
                std::filesystem::path configPath = std::filesystem::path(dir) / "Config";
                if (std::filesystem::exists(configPath))
                {
                    engineConfigDir = std::filesystem::canonical(configPath).string();
                }
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

    // Auto-discover and load all module DLLs from the executable directory.
    // This loads game module DLLs that aren't implicitly linked to the EXE,
    // triggers their FModuleInitializerEntry registration, then calls
    // StartupModule() on each newly discovered module.
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
            FModuleManager::Get().LoadModulesFromDirectory(binDir);
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
