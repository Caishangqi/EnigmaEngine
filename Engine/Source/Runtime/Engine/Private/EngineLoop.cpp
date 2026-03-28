// Copyright EnigmaEngine. All Rights Reserved.

#include "EngineLoop.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "GenericPlatform/GenericApplication.h"
#include "Modules/ModuleManager.h"
#include "Containers/Ticker.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ConfigDelegates.h"

#include "Logging/LogMacros.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

namespace Enigma
{

DEFINE_LOG_CATEGORY_STATIC(LogInit, Info, All);

// ---------------------------------------------------------------
// Plugin descriptor helpers (file-local)
// ---------------------------------------------------------------

/// Map a LoadingPhase string from .eplugin/.eproject to the enum.
static ELoadingPhase ParseLoadingPhase(const std::string& str)
{
    if (str == "EarliestPossible") return ELoadingPhase::EarliestPossible;
    if (str == "PostConfigInit")   return ELoadingPhase::PostConfigInit;
    if (str == "PreLoadingScreen") return ELoadingPhase::PreLoadingScreen;
    if (str == "Default")          return ELoadingPhase::Default;
    if (str == "PostEngineInit")   return ELoadingPhase::PostEngineInit;
    if (str == "None")             return ELoadingPhase::None;
    return ELoadingPhase::PostEngineInit; // safe default for plugins
}

/// Module info extracted from a descriptor.
struct FDescriptorModuleEntry
{
    std::string   Name;
    ELoadingPhase Phase = ELoadingPhase::PostEngineInit;
};

/// Info about an enabled plugin discovered from descriptors.
struct FDiscoveredPlugin
{
    std::string PluginName;
    std::string PluginRoot;   // directory containing .eplugin
    std::vector<FDescriptorModuleEntry> Modules;

    /// Where this plugin was found (matching UE's EPluginLoadedFrom).
    enum class ELoadedFrom : uint8_t
    {
        Engine,   // Found in {EngineRoot}/Plugins/
        Project,  // Found in {ProjectRoot}/Plugins/
    };
    ELoadedFrom LoadedFrom = ELoadedFrom::Engine;
};

/// Find the first .eproject file in the given directory and parse
/// the enabled plugin names from it.
/// Returns {projectModules, enabledPluginNames}.
static std::pair<std::vector<FDescriptorModuleEntry>, std::vector<std::string>>
ParseEProject(const std::filesystem::path& projectRoot)
{
    std::vector<FDescriptorModuleEntry> modules;
    std::vector<std::string> enabledPlugins;

    // Find *.eproject
    std::filesystem::path eprojectPath;
    for (const auto& entry : std::filesystem::directory_iterator(projectRoot))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".eproject")
        {
            eprojectPath = entry.path();
            break;
        }
    }

    if (eprojectPath.empty())
    {
        ENIGMA_LOG(LogInit, Warning, "No .eproject found in '{}'",
            projectRoot.string());
        return {modules, enabledPlugins};
    }

    // Parse JSON
    std::ifstream file(eprojectPath);
    if (!file.is_open())
    {
        ENIGMA_LOG(LogInit, Error, "Failed to open '{}'", eprojectPath.string());
        return {modules, enabledPlugins};
    }

    nlohmann::json doc;
    try
    {
        doc = nlohmann::json::parse(file);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        ENIGMA_LOG(LogInit, Error, "JSON parse error in '{}': {}",
            eprojectPath.string(), e.what());
        return {modules, enabledPlugins};
    }

    ENIGMA_LOG(LogInit, Info, "Parsed project descriptor: {}",
        eprojectPath.string());

    // Extract Modules[]
    if (doc.contains("Modules") && doc["Modules"].is_array())
    {
        for (const auto& m : doc["Modules"])
        {
            FDescriptorModuleEntry entry;
            entry.Name  = m.value("Name", "");
            entry.Phase = ParseLoadingPhase(m.value("LoadingPhase", "Default"));
            if (!entry.Name.empty())
            {
                modules.push_back(std::move(entry));
            }
        }
    }

    // Extract Plugins[] — only enabled ones
    if (doc.contains("Plugins") && doc["Plugins"].is_array())
    {
        for (const auto& p : doc["Plugins"])
        {
            bool enabled = p.value("Enabled", false);
            std::string name = p.value("Name", "");
            if (enabled && !name.empty())
            {
                enabledPlugins.push_back(std::move(name));
            }
        }
    }

    return {modules, enabledPlugins};
}

/// Parse a .eplugin file and return its module entries.
static std::vector<FDescriptorModuleEntry>
ParseEPlugin(const std::filesystem::path& epluginPath)
{
    std::vector<FDescriptorModuleEntry> modules;

    std::ifstream file(epluginPath);
    if (!file.is_open())
    {
        ENIGMA_LOG(LogInit, Error, "Failed to open '{}'", epluginPath.string());
        return modules;
    }

    nlohmann::json doc;
    try
    {
        doc = nlohmann::json::parse(file);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        ENIGMA_LOG(LogInit, Error, "JSON parse error in '{}': {}",
            epluginPath.string(), e.what());
        return modules;
    }

    if (doc.contains("Modules") && doc["Modules"].is_array())
    {
        for (const auto& m : doc["Modules"])
        {
            FDescriptorModuleEntry entry;
            entry.Name  = m.value("Name", "");
            entry.Phase = ParseLoadingPhase(m.value("LoadingPhase", "PostEngineInit"));
            if (!entry.Name.empty())
            {
                modules.push_back(std::move(entry));
            }
        }
    }

    return modules;
}

/// Search for a plugin by name in the given Plugins/ directory.
/// Returns the plugin root directory (containing .eplugin), or empty.
static std::filesystem::path
FindPluginDir(const std::filesystem::path& pluginsDir, const std::string& pluginName)
{
    std::filesystem::path candidate = pluginsDir / pluginName;
    if (!std::filesystem::exists(candidate) || !std::filesystem::is_directory(candidate))
        return {};

    // Verify .eplugin exists
    std::filesystem::path eplugin = candidate / (pluginName + ".eplugin");
    if (std::filesystem::exists(eplugin))
        return candidate;

    return {};
}

/// Discover all enabled plugins from .eproject, parse their .eplugin
/// descriptors, and return structured plugin info.
static std::vector<FDiscoveredPlugin>
DiscoverPlugins(
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& engineRoot)
{
    std::vector<FDiscoveredPlugin> result;

    auto [projectModules, enabledPluginNames] = ParseEProject(projectRoot);

    // Plugin search directories: project first, then engine
    std::filesystem::path projectPluginsDir = projectRoot / "Plugins";
    std::filesystem::path enginePluginsDir  = engineRoot / "Plugins";

    for (const auto& pluginName : enabledPluginNames)
    {
        // Search project plugins first, then engine plugins
        std::filesystem::path pluginDir = FindPluginDir(projectPluginsDir, pluginName);
        FDiscoveredPlugin::ELoadedFrom loadedFrom = FDiscoveredPlugin::ELoadedFrom::Project;
        if (pluginDir.empty())
        {
            pluginDir = FindPluginDir(enginePluginsDir, pluginName);
            loadedFrom = FDiscoveredPlugin::ELoadedFrom::Engine;
        }

        if (pluginDir.empty())
        {
            ENIGMA_LOG(LogInit, Warning,
                "Plugin '{}' is enabled in .eproject but not found in project or engine Plugins/",
                pluginName);
            continue;
        }

        // Parse .eplugin
        std::filesystem::path epluginPath = pluginDir / (pluginName + ".eplugin");
        auto modules = ParseEPlugin(epluginPath);

        FDiscoveredPlugin plugin;
        plugin.PluginName = pluginName;
        plugin.PluginRoot = std::filesystem::canonical(pluginDir).string();
        plugin.Modules    = std::move(modules);
        plugin.LoadedFrom = loadedFrom;

        ENIGMA_LOG(LogInit, Info, "Discovered plugin '{}' at '{}' ({} module(s))",
            plugin.PluginName, plugin.PluginRoot,
            plugin.Modules.size());

        result.push_back(std::move(plugin));
    }

    return result;
}

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
#endif
            }
        }

        // --- Descriptor-driven plugin discovery ---
        // Parse .eproject to find enabled plugins, then parse each .eplugin
        // to discover modules and their loading phases.
        if (!projectConfigDir.empty() && !engineConfigDir.empty())
        {
            std::filesystem::path projectRoot =
                std::filesystem::path(projectConfigDir).parent_path();
            // engineConfigDir = {EngineRoot}/Engine/Config → parent = {EngineRoot}/Engine
            std::filesystem::path engineDir =
                std::filesystem::path(engineConfigDir).parent_path();

            auto plugins = DiscoverPlugins(projectRoot, engineDir);

            for (const auto& plugin : plugins)
            {
                // Add plugin Binaries/ to DLL search path
#ifdef _WIN32
                std::filesystem::path pluginBin =
                    std::filesystem::path(plugin.PluginRoot) / "Binaries" / "Win64";
                if (std::filesystem::exists(pluginBin))
                {
                    std::string pluginBinStr = std::filesystem::canonical(pluginBin).string();
                    FModuleManager::Get().AddDllSearchPath(pluginBinStr);
                    ENIGMA_LOG(LogInit, Info, "Plugin '{}' DLL search path: {}",
                        plugin.PluginName, pluginBinStr);
                }
#endif

                // Register each module to its declared loading phase
                for (const auto& mod : plugin.Modules)
                {
                    AddModuleToPhase(mod.Phase, mod.Name);
                    ENIGMA_LOG(LogInit, Info,
                        "Plugin '{}': registered module '{}' for phase {}",
                        plugin.PluginName, mod.Name,
                        static_cast<int>(mod.Phase));
                }
            }
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

    // Register Developer modules for PostEngineInit (not in Shipping builds).
#if !ENIGMA_BUILD_SHIPPING
    AddModuleToPhase(ELoadingPhase::PostEngineInit, "DirectoryWatcher");
    AddModuleToPhase(ELoadingPhase::PostEngineInit, "HotReload");
#endif

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

    // Ticker tick -- drives DirectoryWatcher, HotReload, etc.
    FTSTicker::GetCoreTicker().Tick(DeltaTime);

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
