// Copyright EnigmaEngine. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInitializerEntry.h"
#include "Logging/LogMacros.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <string>

// Platform DLL loading
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace Enigma
{

DEFINE_LOG_CATEGORY_STATIC(LogModuleManager, Info, All);

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

#ifdef _WIN32
    // Register with Windows so LoadLibrary can resolve dependent DLLs
    // across directories (e.g. game DLL depending on plugin DLL in
    // a different Binaries/ folder).
    static bool bDefaultDirsSet = false;
    if (!bDefaultDirsSet)
    {
        // Enable user-added directories in the default DLL search order.
        // LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = APPLICATION_DIR | SYSTEM32 | USER_DIRS
        ::SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        bDefaultDirsSet = true;
    }

    // Convert to wide string for AddDllDirectoryW
    int wlen = ::MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen > 0)
    {
        std::wstring wpath(wlen - 1, L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), wlen);
        ::AddDllDirectory(wpath.c_str());
    }
#endif
}

// ---------------------------------------------------------------
// LoadModule
// ---------------------------------------------------------------
IModuleInterface* FModuleManager::LoadModule(std::string_view name)
{
    std::string key(name);
    IModuleInterface* rawModule = nullptr;
    void* dllHandle = nullptr;
    std::string dllFilePath;

    // Phase 1: Find/load DLL and create module instance (under lock).
    {
        std::lock_guard<std::mutex> lock(Mutex);

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

        if (!initFn)
        {
            // Step 2: Entry not found -- try to load the DLL explicitly.
            std::string dllName = PlatformDllPath(name);
            dllHandle = PlatformLoadDll(dllName.c_str());
            if (dllHandle)
            {
                dllFilePath = dllName;
            }

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
                        dllFilePath = fullPath;
                        ENIGMA_LOG(LogModuleManager, Info, "Loaded '{}' from search path '{}'",
                            key, searchPath);
                        break;
                    }
                }
            }

            if (!dllHandle)
            {
                ENIGMA_LOG(LogModuleManager, Error,
                    "Failed to load DLL '{}' (searched {} additional path(s))",
                    dllName, DllSearchPaths.size());
                return nullptr;
            }

            // Now find the initializer (DLL static constructors should have registered it).
            initFn = FModuleInitializerEntry::FindModule(key.c_str());
            if (!initFn)
            {
                ENIGMA_LOG(LogModuleManager, Error,
                    "Module '{}' loaded but no initializer found (missing IMPLEMENT_MODULE?)",
                    key);
                PlatformFreeDll(dllHandle);
                return nullptr;
            }
        }

        // Step 3: Create the module instance.
        rawModule = initFn();
        if (!rawModule)
        {
            ENIGMA_LOG(LogModuleManager, Error, "Initializer for '{}' returned null", key);
            if (dllHandle) PlatformFreeDll(dllHandle);
            return nullptr;
        }

        // Step 4: Store in map BEFORE calling StartupModule (matching UE pattern).
        // This allows StartupModule to call IsModuleLoaded/GetModule on other modules.
        FModuleInfo info;
        info.Name        = key;
        info.DllHandle   = dllHandle;
        info.Module      = std::unique_ptr<IModuleInterface>(rawModule);
        info.LoadOrder   = NextLoadOrder++;
        info.DllFilePath = std::move(dllFilePath);

        Modules.emplace(key, std::move(info));
    }
    // Lock released here.

    // Step 5: Call StartupModule WITHOUT holding the lock (matching UE pattern).
    // This allows StartupModule to call LoadModule/IsModuleLoaded without deadlock.
    rawModule->StartupModule();

    return rawModule;
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
// ScanDllsFromDirectory
// ---------------------------------------------------------------
void FModuleManager::ScanDllsFromDirectory(const std::string& directory)
{
    // Load DLLs from the directory into the process.
    // This triggers static FModuleInitializerEntry registration
    // but does NOT call StartupModule / initialize modules.
    //
    // If a .modules manifest is found for the current TargetName,
    // only the DLLs listed in the manifest are loaded. This prevents
    // loading DLLs from a different build configuration (e.g. loading
    // both Development and DebugGame DLLs from the same directory).

#ifdef _WIN32
    // --- Phase 1: Try manifest-based loading ---
    if (!TargetName.empty())
    {
        std::string manifestPath = directory;
        if (!manifestPath.empty() && manifestPath.back() != '\\' && manifestPath.back() != '/')
            manifestPath += '\\';
        manifestPath += TargetName + ".modules";

        std::ifstream manifestFile(manifestPath);
        if (manifestFile.is_open())
        {
            ENIGMA_LOG(LogModuleManager, Info, "Loading modules from manifest: {}", manifestPath);

            // Simple line-by-line parsing: extract "xxx.dll" values
            // from the JSON without a full JSON parser.
            // Format: "ModuleName": "SomeFile.dll"
            std::string line;
            std::vector<std::string> dllNames;
            while (std::getline(manifestFile, line))
            {
                // Look for lines containing .dll"
                auto dllEnd = line.find(".dll\"");
                if (dllEnd == std::string::npos)
                    continue;

                // Walk backwards from .dll" to find the opening quote
                // of the value: "SomeFile.dll"
                auto valueEnd = dllEnd + 4; // position after ".dll"
                // Find the quote before the DLL filename
                auto valueStart = line.rfind('"', dllEnd - 1);
                if (valueStart == std::string::npos)
                    continue;

                std::string dllName = line.substr(valueStart + 1, valueEnd - valueStart - 1);
                if (!dllName.empty())
                {
                    dllNames.push_back(dllName);
                }
            }
            manifestFile.close();

            // Load only the DLLs listed in the manifest
            for (const auto& dllName : dllNames)
            {
                std::string fullPath = directory;
                if (!fullPath.empty() && fullPath.back() != '\\' && fullPath.back() != '/')
                    fullPath += '\\';
                fullPath += dllName;

                void* handle = PlatformLoadDll(fullPath.c_str());
                if (!handle)
                {
                    DWORD err = ::GetLastError();
                    ENIGMA_LOG(LogModuleManager, Warning,
                        "Could not load DLL '{}' (GetLastError={})", fullPath, err);
                }
            }

            return; // Manifest found and processed -- skip wildcard scan
        }
    }

    // --- Phase 2: Fallback to *.dll wildcard scan ---
    ENIGMA_LOG(LogModuleManager, Info, "No manifest found, scanning *.dll in: {}", directory);

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
            if (!handle)
            {
                DWORD err = ::GetLastError();
                ENIGMA_LOG(LogModuleManager, Error,
                    "Failed to load DLL '{}' (GetLastError={})", fullPath, err);
            }
        } while (::FindNextFileA(hFind, &fd));
        ::FindClose(hFind);
    }
#else
    // POSIX: use opendir/readdir
    // (not implemented yet -- game modules on Linux/Mac would need this)
#endif
}

// ---------------------------------------------------------------
// LoadModulesFromDirectory
// ---------------------------------------------------------------
void FModuleManager::LoadModulesFromDirectory(const std::string& directory)
{
    // Phase 1: Load all DLLs (triggers static registration).
    ScanDllsFromDirectory(directory);

    // Phase 2: Initialize all newly registered modules.
    LoadAllRegisteredModules();

    // Note: DLL handles are intentionally NOT freed here.
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

// ---------------------------------------------------------------
// LoadDllFromPath -- public wrapper for PlatformLoadDll
// ---------------------------------------------------------------
void* FModuleManager::LoadDllFromPath(const char* path)
{
    return PlatformLoadDll(path);
}

// ---------------------------------------------------------------
// GetModuleDllPath
// ---------------------------------------------------------------
std::string FModuleManager::GetModuleDllPath(std::string_view name) const
{
    std::lock_guard<std::mutex> lock(Mutex);
    std::string key(name);
    auto it = Modules.find(key);
    if (it != Modules.end())
    {
        return it->second.DllFilePath;
    }
    return {};
}

} // namespace Enigma