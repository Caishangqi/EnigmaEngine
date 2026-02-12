// Copyright EnigmaEngine. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInitializerEntry.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

// Platform DLL loading
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace Enigma
{

// ---------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------
FModuleManager& FModuleManager::Get()
{
    static FModuleManager instance;
    return instance;
}

FModuleManager::~FModuleManager()
{
    UnloadAllModules();
}

// ---------------------------------------------------------------
// Platform helpers
// ---------------------------------------------------------------
void* FModuleManager::PlatformLoadDll(const char* path)
{
#ifdef _WIN32
    return static_cast<void*>(::LoadLibraryA(path));
#else
    return ::dlopen(path, RTLD_NOW);
#endif
}

void FModuleManager::PlatformFreeDll(void* handle)
{
    if (!handle) return;
#ifdef _WIN32
    ::FreeLibrary(static_cast<HMODULE>(handle));
#else
    ::dlclose(handle);
#endif
}

std::string FModuleManager::PlatformDllPath(std::string_view moduleName)
{
    std::string name(moduleName);
#ifdef _WIN32
    return name + ".dll";
#elif defined(__APPLE__)
    return "lib" + name + ".dylib";
#else
    return "lib" + name + ".so";
#endif
}

// ---------------------------------------------------------------
// DLL search paths
// ---------------------------------------------------------------
void FModuleManager::AddDllSearchPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(Mutex);
    DllSearchPaths.push_back(path);
}

// ---------------------------------------------------------------
// LoadModule
// ---------------------------------------------------------------
IModuleInterface* FModuleManager::LoadModule(std::string_view name)
{
    std::lock_guard<std::mutex> lock(Mutex);

    std::string key(name);

    // Already loaded? Return existing instance.
    auto it = Modules.find(key);
    if (it != Modules.end() && it->second.Module)
    {
        return it->second.Module.get();
    }

    // Step 1: Check if the module initializer is already registered
    //         (DLL may have been implicitly linked by the executable).
    FInitializeModuleFunctionPtr initFn =
        FModuleInitializerEntry::FindModule(key.c_str());

    void* dllHandle = nullptr;

    if (!initFn)
    {
        // Step 2: Entry not found -- try to load the DLL explicitly.
        std::string dllName = PlatformDllPath(name);
        dllHandle = PlatformLoadDll(dllName.c_str());

        // If default path failed, try each registered search path.
        if (!dllHandle)
        {
            for (const auto& searchPath : DllSearchPaths)
            {
                std::string fullPath = searchPath;
                if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\')
                {
                    fullPath += '/';
                }
                fullPath += dllName;

                dllHandle = PlatformLoadDll(fullPath.c_str());
                if (dllHandle)
                {
                    std::printf("[FModuleManager] Loaded '%s' from search path '%s'\n",
                        key.c_str(), searchPath.c_str());
                    break;
                }
            }
        }

        if (!dllHandle)
        {
            std::fprintf(stderr,
                "[FModuleManager] ERROR: Failed to load DLL '%s'"
                " (searched %zu additional path(s))\n",
                dllName.c_str(),
                DllSearchPaths.size());
            return nullptr;
        }

        // Now find the initializer (DLL static constructors should have registered it).
        initFn = FModuleInitializerEntry::FindModule(key.c_str());
        if (!initFn)
        {
            std::fprintf(stderr,
                "[FModuleManager] ERROR: Module '%s' loaded but no "
                "initializer found (missing IMPLEMENT_MODULE?)\n",
                key.c_str());
            PlatformFreeDll(dllHandle);
            return nullptr;
        }
    }

    // Step 3: Create the module instance.
    IModuleInterface* rawModule = initFn();
    if (!rawModule)
    {
        std::fprintf(stderr,
            "[FModuleManager] ERROR: Initializer for '%s' returned null\n",
            key.c_str());
        if (dllHandle) PlatformFreeDll(dllHandle);
        return nullptr;
    }

    // Step 4: Call StartupModule and store.
    rawModule->StartupModule();

    FModuleInfo info;
    info.Name      = key;
    info.DllHandle = dllHandle;  // nullptr for implicitly linked modules
    info.Module    = std::unique_ptr<IModuleInterface>(rawModule);
    info.LoadOrder = NextLoadOrder++;

    auto [insertIt, _] = Modules.emplace(key, std::move(info));
    return insertIt->second.Module.get();
}

// ---------------------------------------------------------------
// LoadAllRegisteredModules
// ---------------------------------------------------------------
void FModuleManager::LoadAllRegisteredModules()
{
    // Collect all registered module names first (avoid calling LoadModule
    // while iterating the entry list, since LoadModule acquires the mutex).
    std::vector<std::string> names;
    FModuleInitializerEntry::ForEach(
        [](const char* name, void* userData) {
            static_cast<std::vector<std::string>*>(userData)->emplace_back(name);
        },
        &names);

    for (const auto& name : names)
    {
        if (!IsModuleLoaded(name))
        {
            LoadModule(name);
        }
    }
}

// ---------------------------------------------------------------
// LoadModulesFromDirectory
// ---------------------------------------------------------------
void FModuleManager::LoadModulesFromDirectory(const std::string& directory)
{
    // Phase 1: Load all DLLs from the directory into the process.
    //          This triggers static FModuleInitializerEntry registration.
    std::vector<void*> loadedHandles;

#ifdef _WIN32
    std::string pattern = directory;
    if (!pattern.empty() && pattern.back() != '\\' && pattern.back() != '/')
        pattern += '\\';
    pattern += "*.dll";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = ::FindFirstFileA(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::string fullPath = directory;
            if (!fullPath.empty() && fullPath.back() != '\\' && fullPath.back() != '/')
                fullPath += '\\';
            fullPath += fd.cFileName;

            void* handle = PlatformLoadDll(fullPath.c_str());
            if (handle)
            {
                loadedHandles.push_back(handle);
            }
        } while (::FindNextFileA(hFind, &fd));
        ::FindClose(hFind);
    }
#else
    // POSIX: use opendir/readdir
    // (not implemented yet -- game modules on Linux/Mac would need this)
#endif

    // Phase 2: Initialize all newly registered modules.
    LoadAllRegisteredModules();

    // Note: DLL handles from loadedHandles are intentionally NOT freed here.
    // The modules remain loaded for the lifetime of the process.
    // UnloadAllModules() will handle cleanup for modules tracked by FModuleInfo.
    // For DLLs loaded here but not claimed by any module entry, they stay loaded
    // (harmless -- they're just DLLs without IMPLEMENT_MODULE).
}

// ---------------------------------------------------------------
// UnloadModule
// ---------------------------------------------------------------
bool FModuleManager::UnloadModule(std::string_view name)
{
    std::lock_guard<std::mutex> lock(Mutex);

    std::string key(name);
    auto it = Modules.find(key);
    if (it == Modules.end() || !it->second.Module)
    {
        return false;
    }

    FModuleInfo& info = it->second;

    // Step 1: Shutdown the module.
    info.Module->ShutdownModule();

    // Step 2: Destroy the module object (before DLL unload!).
    info.Module.reset();

    // Step 3: Free the DLL.
    if (info.DllHandle)
    {
        PlatformFreeDll(info.DllHandle);
        info.DllHandle = nullptr;
    }

    // Step 4: Remove from map.
    Modules.erase(it);
    return true;
}

// ---------------------------------------------------------------
// UnloadAllModules -- reverse load-order (LIFO)
// ---------------------------------------------------------------
void FModuleManager::UnloadAllModules()
{
    std::lock_guard<std::mutex> lock(Mutex);

    // Collect modules and sort by descending LoadOrder.
    std::vector<std::string> ordered;
    ordered.reserve(Modules.size());
    for (auto& [k, _] : Modules)
    {
        ordered.push_back(k);
    }
    std::sort(ordered.begin(), ordered.end(),
        [this](const std::string& a, const std::string& b)
        {
            return Modules[a].LoadOrder > Modules[b].LoadOrder;
        });

    // Shutdown all first, then destroy + free DLLs.
    for (auto& name : ordered)
    {
        auto it = Modules.find(name);
        if (it != Modules.end() && it->second.Module)
        {
            it->second.Module->ShutdownModule();
        }
    }

    for (auto& name : ordered)
    {
        auto it = Modules.find(name);
        if (it == Modules.end()) continue;

        it->second.Module.reset();

        if (it->second.DllHandle)
        {
            PlatformFreeDll(it->second.DllHandle);
            it->second.DllHandle = nullptr;
        }
    }

    Modules.clear();
}

// ---------------------------------------------------------------
// Queries
// ---------------------------------------------------------------
bool FModuleManager::IsModuleLoaded(std::string_view name) const
{
    std::lock_guard<std::mutex> lock(Mutex);
    std::string key(name);
    auto it = Modules.find(key);
    return it != Modules.end() && it->second.Module != nullptr;
}

IModuleInterface* FModuleManager::GetModule(std::string_view name)
{
    std::lock_guard<std::mutex> lock(Mutex);
    std::string key(name);
    auto it = Modules.find(key);
    if (it != Modules.end() && it->second.Module)
    {
        return it->second.Module.get();
    }
    return nullptr;
}

} // namespace Enigma