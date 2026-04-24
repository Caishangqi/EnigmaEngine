// Copyright EnigmaEngine. All Rights Reserved.

#include "IHotReload.h"
#include "Containers/Ticker.h"
#include "IDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleMacros.h"
#include "Logging/LogMacros.h"

// [TEST] For GameInstance recreation after hot-reload. Remove when Editor exists.
#include "Engine/GameEngine.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Enigma
{

DEFINE_LOG_CATEGORY_STATIC(LogHotReload, Info, All);

// ---------------------------------------------------------------
// Hot-reload suffix helpers (matching UE's -NNNN pattern)
// ---------------------------------------------------------------

/// Check if a filename stem ends with a hot-reload suffix (-NNNN).
static bool HasHotReloadSuffix(const std::string& Stem)
{
    if (Stem.size() < 5) return false;
    if (Stem[Stem.size() - 5] != '-') return false;
    for (int i = 1; i <= 4; ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(Stem[Stem.size() - i])))
            return false;
    }
    return true;
}

/// Strip the -NNNN suffix from a filename stem.
static std::string StripHotReloadSuffix(const std::string& Stem)
{
    if (!HasHotReloadSuffix(Stem)) return Stem;
    return Stem.substr(0, Stem.size() - 5);
}

/// Extract the numeric suffix value from a filename stem.
static int ExtractHotReloadSuffix(const std::string& Stem)
{
    if (!HasHotReloadSuffix(Stem)) return -1;
    return std::stoi(Stem.substr(Stem.size() - 4));
}

// ---------------------------------------------------------------
// FHotReloadModule -- versioned DLL hot-reload (matching UE)
// ---------------------------------------------------------------
class FHotReloadModule : public IHotReload
{
public:
    // --- IModuleInterface ---
    void StartupModule() override;
    void ShutdownModule() override;
    bool SupportsDynamicReloading() override { return false; }

    // --- IHotReload ---
    bool ReloadModule(std::string_view ModuleName) override;
    bool IsReloading() const override { return bIsReloading; }
    void SetEnabled(bool bInEnabled) override { bEnabled = bInEnabled; }
    bool IsEnabled() const override { return bEnabled; }

    TMulticastDelegate<std::string_view>& OnModuleReloaded() override
    {
        return ModuleReloadedDelegate;
    }

    TMulticastDelegate<std::string_view, std::string_view>& OnReloadFailed() override
    {
        return ReloadFailedDelegate;
    }

private:
    bool Tick(float DeltaTime);
    void OnBinariesChanged(const std::vector<FFileChangeData>& Changes);
    void RefreshWatchedDirectories();
    void CleanStaleHotReloadFiles();
    bool DoReloadModule(const std::string& InModuleName,
                        const std::string& VersionedDllPath);

    // State
    bool                    bEnabled = true;
    bool                    bIsReloading = false;
    FDelegateHandle         TickerHandle;

    // Detected changes
    struct FDetectedChange
    {
        std::string ModuleName;
        std::string VersionedDllPath;
        double      DetectedTime = 0.0;
    };
    std::vector<FDetectedChange> DetectedModules;
    double                       DebounceDelay = 0.5;

    // DirectoryWatcher handles for cleanup
    std::vector<std::pair<std::string, FDelegateHandle>> WatchHandles;

    // Events
    TMulticastDelegate<std::string_view>                    ModuleReloadedDelegate;
    TMulticastDelegate<std::string_view, std::string_view>  ReloadFailedDelegate;

    // Time tracking
    double CurrentTime = 0.0;
};

// ---------------------------------------------------------------
// StartupModule / ShutdownModule
// ---------------------------------------------------------------

void FHotReloadModule::StartupModule()
{
    FTickerDelegate TickDelegate;
    TickDelegate.Bind(this, &FHotReloadModule::Tick);
    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);

    // Force-load DirectoryWatcher before setting up watches (matching UE pattern).
    // LoadModuleChecked ensures DirectoryWatcher is loaded and initialized
    // before we try to register callbacks with it.
    FModuleManager::Get().LoadModule("DirectoryWatcher");

    // Clean versioned DLLs/PDBs left over from previous hot-reload sessions.
    // At startup no old DLLs are loaded, so they can be safely deleted.
    CleanStaleHotReloadFiles();

    RefreshWatchedDirectories();

    // [TEST] Register callback to recreate GameInstance after hot-reload.
    // Without an Editor, existing object instances have stale vtables after
    // module reload. This workaround destroys and recreates the GameInstance
    // so the new DLL's code takes effect. Remove when Editor exists.
    ModuleReloadedDelegate.Add([](std::string_view ModuleName)
    {
        if (!GEngine) return;

        auto* GameEngine = dynamic_cast<FGameEngine*>(GEngine);
        if (!GameEngine || !GameEngine->GetGameInstance()) return;

        GameEngine->RecreateGameInstance();
    });

    ENIGMA_LOG(LogHotReload, Info, "HotReload module started (enabled={})",
        bEnabled ? "true" : "false");
}

void FHotReloadModule::ShutdownModule()
{
    FTSTicker::RemoveTicker(TickerHandle);

    if (FModuleManager::Get().IsModuleLoaded("DirectoryWatcher"))
    {
        auto& DWModule = FModuleManager::Get().GetModuleChecked<FDirectoryWatcherModule>(
            "DirectoryWatcher");
        auto* Watcher = DWModule.Get();
        if (Watcher)
        {
            for (auto& [Dir, Handle] : WatchHandles)
            {
                Watcher->UnregisterDirectoryChangedCallback(Dir, Handle);
            }
        }
    }
    WatchHandles.clear();

    ENIGMA_LOG(LogHotReload, Info, "HotReload module shut down");
}

// ---------------------------------------------------------------
// RefreshWatchedDirectories
// ---------------------------------------------------------------

void FHotReloadModule::RefreshWatchedDirectories()
{
    auto& ModMgr = FModuleManager::Get();
    if (!ModMgr.IsModuleLoaded("DirectoryWatcher"))
    {
        return;
    }

    auto& DWModule = ModMgr.GetModuleChecked<FDirectoryWatcherModule>("DirectoryWatcher");
    auto* Watcher = DWModule.Get();
    if (!Watcher)
    {
        return;
    }

    // Unregister old watches.
    for (auto& [Dir, Handle] : WatchHandles)
    {
        Watcher->UnregisterDirectoryChangedCallback(Dir, Handle);
    }
    WatchHandles.clear();

    // Watch each DLL search path (project Binaries/, plugin Binaries/).
    // These are the directories where versioned hot-reload DLLs will appear.
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind(this, &FHotReloadModule::OnBinariesChanged);

    for (const auto& SearchPath : ModMgr.GetDllSearchPaths())
    {
        FDelegateHandle Handle;
        if (Watcher->RegisterDirectoryChangedCallback(SearchPath, Callback, Handle, 0))
        {
            WatchHandles.emplace_back(SearchPath, Handle);
            ENIGMA_LOG(LogHotReload, Info, "Watching: {}", SearchPath);
        }
    }
}

// ---------------------------------------------------------------
// CleanStaleHotReloadFiles -- remove old versioned DLLs/PDBs
// ---------------------------------------------------------------

void FHotReloadModule::CleanStaleHotReloadFiles()
{
    namespace fs = std::filesystem;

    auto& ModMgr = FModuleManager::Get();
    const auto& SearchPaths = ModMgr.GetDllSearchPaths();

    int Removed = 0;
    for (const auto& Dir : SearchPaths)
    {
        if (!fs::exists(Dir) || !fs::is_directory(Dir))
            continue;

        std::error_code EC;
        for (const auto& Entry : fs::directory_iterator(Dir, EC))
        {
            if (!Entry.is_regular_file()) continue;

            auto Ext = Entry.path().extension().string();
            if (Ext != ".dll" && Ext != ".pdb") continue;

            std::string Stem = Entry.path().stem().string();
            if (!HasHotReloadSuffix(Stem)) continue;

            std::error_code RemoveEC;
            if (fs::remove(Entry.path(), RemoveEC))
            {
                ++Removed;
            }
            else if (RemoveEC)
            {
                ENIGMA_LOG(LogHotReload, Warning,
                    "Could not remove stale {}: {}",
                    Entry.path().filename().string(), RemoveEC.message());
            }
        }
    }

    if (Removed > 0)
    {
        ENIGMA_LOG(LogHotReload, Info,
            "Cleaned {} stale hot-reload artifact(s)", Removed);
    }
}

// ---------------------------------------------------------------
// Tick -- per-frame check for pending reloads
// ---------------------------------------------------------------

bool FHotReloadModule::Tick(float DeltaTime)
{
    CurrentTime += static_cast<double>(DeltaTime);

    if (!bEnabled || bIsReloading || DetectedModules.empty())
    {
        return true;
    }

    double LatestDetection = 0.0;
    for (const auto& Change : DetectedModules)
    {
        LatestDetection = (std::max)(LatestDetection, Change.DetectedTime);
    }

    if ((CurrentTime - LatestDetection) < DebounceDelay)
    {
        return true;
    }

    bIsReloading = true;

    auto PendingModules = std::move(DetectedModules);
    DetectedModules.clear();

    // Deduplicate by module name (keep latest versioned DLL).
    std::unordered_map<std::string, FDetectedChange> UniqueModules;
    for (auto& Change : PendingModules)
    {
        UniqueModules[Change.ModuleName] = std::move(Change);
    }

    for (auto& [Name, Change] : UniqueModules)
    {
        ENIGMA_LOG(LogHotReload, Info, "Reloading module '{}' from '{}'...",
            Name, Change.VersionedDllPath);
        DoReloadModule(Name, Change.VersionedDllPath);
    }

    bIsReloading = false;
    return true;
}

// ---------------------------------------------------------------
// OnBinariesChanged -- detect versioned DLLs (-NNNN.dll)
// ---------------------------------------------------------------

void FHotReloadModule::OnBinariesChanged(
    const std::vector<FFileChangeData>& Changes)
{
    if (!bEnabled)
    {
        return;
    }

    auto& ModMgr = FModuleManager::Get();

    for (const auto& Change : Changes)
    {
        // Filter: only FCA_Added events.
        if (Change.Action != FFileChangeData::FCA_Added)
        {
            continue;
        }

        // Filter: only .dll files.
        std::filesystem::path FilePath(Change.Filename);
        if (FilePath.extension() != ".dll")
        {
            continue;
        }

        std::string Stem = FilePath.stem().string();

        // Only process files WITH a hot-reload suffix (-NNNN).
        if (!HasHotReloadSuffix(Stem))
        {
            continue;
        }

        // Strip suffix to get original base name.
        std::string OriginalStem = StripHotReloadSuffix(Stem);

        // Extract module name from "EnigmaEngine-{ModuleName}[-Platform-Config]".
        std::string ModuleName;
        const std::string EnginePrefix = "EnigmaEngine-";
        if (OriginalStem.starts_with(EnginePrefix))
        {
            std::string Remainder = OriginalStem.substr(EnginePrefix.size());
            auto DashPos = Remainder.find('-');
            if (DashPos != std::string::npos)
            {
                ModuleName = Remainder.substr(0, DashPos);
            }
            else
            {
                ModuleName = Remainder;
            }
        }

        if (ModuleName.empty())
        {
            continue;
        }

        // Verify module is loaded and supports reloading.
        if (!ModMgr.IsModuleLoaded(ModuleName))
        {
            continue;
        }

        auto* Module = ModMgr.GetModule(ModuleName);
        if (!Module || !Module->SupportsDynamicReloading())
        {
            continue;
        }

        ENIGMA_LOG(LogHotReload, Info,
            "Detected versioned DLL for module '{}': {}",
            ModuleName, Change.Filename);

        FDetectedChange Detected;
        Detected.ModuleName = ModuleName;
        Detected.VersionedDllPath = Change.Filename;
        Detected.DetectedTime = CurrentTime;
        DetectedModules.push_back(std::move(Detected));
    }
}

// ---------------------------------------------------------------
// ReloadModule -- public manual trigger
// ---------------------------------------------------------------

bool FHotReloadModule::ReloadModule(std::string_view ModuleName)
{
    namespace fs = std::filesystem;
    std::string Name(ModuleName);

    // Get the module's current DLL path to find the Binaries directory.
    std::string CurrentPath = FModuleManager::Get().GetModuleDllPath(Name);
    if (CurrentPath.empty())
    {
        std::string Err = "Module not loaded or DLL path unknown";
        ENIGMA_LOG(LogHotReload, Warning, "Cannot reload '{}': {}", Name, Err);
        ReloadFailedDelegate.Broadcast(std::string_view(Name), std::string_view(Err));
        return false;
    }

    // Scan directory for the highest versioned DLL.
    fs::path Dir = fs::path(CurrentPath).parent_path();
    std::string BaseStem = fs::path(CurrentPath).stem().string();
    if (HasHotReloadSuffix(BaseStem))
    {
        BaseStem = StripHotReloadSuffix(BaseStem);
    }

    std::string BestPath;
    int BestSuffix = -1;
    if (fs::exists(Dir))
    {
        for (const auto& Entry : fs::directory_iterator(Dir))
        {
            if (!Entry.is_regular_file()) continue;
            if (Entry.path().extension() != ".dll") continue;

            std::string EntryStem = Entry.path().stem().string();
            if (!HasHotReloadSuffix(EntryStem)) continue;

            std::string EntryBase = StripHotReloadSuffix(EntryStem);
            if (EntryBase != BaseStem) continue;

            int Suffix = ExtractHotReloadSuffix(EntryStem);
            if (Suffix > BestSuffix)
            {
                BestSuffix = Suffix;
                BestPath = Entry.path().string();
            }
        }
    }

    if (BestPath.empty())
    {
        std::string Err = "No versioned DLL found in " + Dir.string();
        ENIGMA_LOG(LogHotReload, Warning, "Cannot reload '{}': {}", Name, Err);
        ReloadFailedDelegate.Broadcast(std::string_view(Name), std::string_view(Err));
        return false;
    }

    ENIGMA_LOG(LogHotReload, Info, "Manual reload '{}' from '{}'", Name, BestPath);
    return DoReloadModule(Name, BestPath);
}

// ---------------------------------------------------------------
// DoReloadModule -- versioned DLL reload sequence
// ---------------------------------------------------------------

bool FHotReloadModule::DoReloadModule(
    const std::string& InModuleName,
    const std::string& VersionedDllPath)
{
    auto& ModMgr = FModuleManager::Get();

    auto* Module = ModMgr.GetModule(InModuleName);
    if (!Module)
    {
        std::string Err = "Module not loaded";
        ENIGMA_LOG(LogHotReload, Warning, "Cannot reload '{}': {}", InModuleName, Err);
        ReloadFailedDelegate.Broadcast(
            std::string_view(InModuleName), std::string_view(Err));
        return false;
    }

    if (!Module->SupportsDynamicReloading())
    {
        std::string Err = "Module does not support dynamic reloading";
        ENIGMA_LOG(LogHotReload, Warning, "Cannot reload '{}': {}", InModuleName, Err);
        ReloadFailedDelegate.Broadcast(
            std::string_view(InModuleName), std::string_view(Err));
        return false;
    }

    // Step 1: Unload old module (ShutdownModule + FreeLibrary).
    if (!ModMgr.UnloadModule(InModuleName))
    {
        std::string Err = "Failed to unload module";
        ENIGMA_LOG(LogHotReload, Error, "Reload '{}' failed: {}", InModuleName, Err);
        ReloadFailedDelegate.Broadcast(
            std::string_view(InModuleName), std::string_view(Err));
        return false;
    }

    // Step 2: Load the new versioned DLL into the process.
    void* NewHandle = FModuleManager::LoadDllFromPath(VersionedDllPath.c_str());
    if (!NewHandle)
    {
        std::string Err = "Failed to load versioned DLL: " + VersionedDllPath;
        ENIGMA_LOG(LogHotReload, Error, "Reload '{}' failed: {}", InModuleName, Err);
        ReloadFailedDelegate.Broadcast(
            std::string_view(InModuleName), std::string_view(Err));
        return false;
    }

    // Step 3: Initialize the module (finds new FModuleInitializerEntry, calls StartupModule).
    auto* NewModule = ModMgr.LoadModule(InModuleName);
    if (!NewModule)
    {
        std::string Err = "Failed to initialize module after DLL load";
        ENIGMA_LOG(LogHotReload, Error, "Reload '{}' failed: {}", InModuleName, Err);
        ReloadFailedDelegate.Broadcast(
            std::string_view(InModuleName), std::string_view(Err));
        return false;
    }

    ENIGMA_LOG(LogHotReload, Info, "Module '{}' reloaded successfully from '{}'",
        InModuleName, VersionedDllPath);
    ModuleReloadedDelegate.Broadcast(std::string_view(InModuleName));
    return true;
}

IMPLEMENT_MODULE(FHotReloadModule, HotReload)

} // namespace Enigma
