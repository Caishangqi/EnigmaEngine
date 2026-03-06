// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "CoreAPI.generated.h"
#include "Modules/ModuleInterface.h"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cassert>

namespace Enigma
{

// ---------------------------------------------------------------
// FModuleInfo -- per-module runtime state
// ---------------------------------------------------------------
struct FModuleInfo
{
    std::string              Name;
    void*                    DllHandle = nullptr;
    std::unique_ptr<IModuleInterface> Module;
    int32_t                  LoadOrder = 0;
};

// ---------------------------------------------------------------
// FModuleManager -- singleton that owns all loaded modules
//
// Mirrors Unreal Engine's FModuleManager at a minimal scope:
//   - LoadModule:  LoadLibrary + FModuleInitializerEntry::FindModule
//   - UnloadModule: ShutdownModule + FreeLibrary
//   - GetModule / GetModuleChecked<T>
//   - IsModuleLoaded
//   - UnloadAllModules (reverse load-order shutdown)
//
// Thread safety: the Modules map is protected by a std::mutex.
// ---------------------------------------------------------------
class CORE_API FModuleManager
{
public:
    // Meyer's singleton -- thread-safe per C++11.
    static FModuleManager& Get();

    // Non-copyable, non-movable.
    FModuleManager(const FModuleManager&) = delete;
    FModuleManager& operator=(const FModuleManager&) = delete;

    // ----- Module lifecycle -----

    /// Load a module by name.
    /// If already loaded, returns the existing instance.
    /// On failure returns nullptr and logs an error.
    IModuleInterface* LoadModule(std::string_view name);

    /// Discover and load all modules whose DLLs are already loaded
    /// (implicitly linked) but not yet initialized via LoadModule().
    /// Calls StartupModule() on each newly loaded module.
    void LoadAllRegisteredModules();

    /// Load all DLLs from the given directory into the process,
    /// then initialize any newly registered modules.
    /// Used at startup to discover game/plugin module DLLs.
    void LoadModulesFromDirectory(const std::string& directory);

    /// Load all DLLs from the given directory into the process
    /// WITHOUT initializing modules. Use with LoadAllRegisteredModules()
    /// for two-phase loading when cross-directory DLL dependencies exist.
    void ScanDllsFromDirectory(const std::string& directory);

    /// Unload a single module by name.
    /// Calls ShutdownModule(), destroys the module object, frees the DLL.
    /// Returns true if the module was found and unloaded.
    bool UnloadModule(std::string_view name);

    /// Unload all modules in reverse load-order (LIFO).
    /// Called during engine shutdown.
    void UnloadAllModules();

    // ----- Queries -----

    /// Returns true if the module is currently loaded.
    bool IsModuleLoaded(std::string_view name) const;

    /// Returns the module interface pointer, or nullptr if not loaded.
    IModuleInterface* GetModule(std::string_view name);

    /// Returns a typed reference to a loaded module.
    /// Asserts (and crashes) if the module is not loaded.
    template <typename T>
    T& GetModuleChecked(std::string_view name);

    // ----- DLL search paths -----

    /// Add a directory to the DLL search path list.
    /// When LoadModule fails to find a DLL in the working directory,
    /// it will try each search path in order.
    /// Typical use: add plugin Binaries/ directories.
    void AddDllSearchPath(const std::string& path);

    /// Returns the current DLL search paths (read-only).
    const std::vector<std::string>& GetDllSearchPaths() const { return DllSearchPaths; }

private:
    FModuleManager() = default;
    ~FModuleManager();

    // Platform DLL helpers
    static void*  PlatformLoadDll(const char* path);
    static void   PlatformFreeDll(void* handle);
    static std::string PlatformDllPath(std::string_view moduleName);

    mutable std::mutex                              Mutex;
    std::unordered_map<std::string, FModuleInfo>    Modules;
    std::vector<std::string>                        DllSearchPaths;
    int32_t                                         NextLoadOrder = 0;
};

// ---------------------------------------------------------------
// Template implementation (must be in header)
// ---------------------------------------------------------------
template <typename T>
T& FModuleManager::GetModuleChecked(std::string_view name)
{
    IModuleInterface* mod = GetModule(name);
    assert(mod && "GetModuleChecked: module is not loaded");
    return static_cast<T&>(*mod);
}

} // namespace Enigma
