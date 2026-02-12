// Copyright EnigmaEngine. All Rights Reserved.

#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Modules/ModuleManager.h"

#include <cstdio>

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

    // Phase 1: PostConfigInit (config-dependent modules)
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

    // Phase 4: PostEngineInit (plugins, late modules)
    LoadModulesForPhase(ELoadingPhase::PostEngineInit);

    // Initialize engine (uses registered factories, e.g. CreateGameInstance)
    GEngine->Init(this);

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

    // Unload all modules in reverse load-order
    FModuleManager::Get().UnloadAllModules();

    std::printf("[FEngineLoop] Exit complete\n");
}

} // namespace Enigma
