// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "Delegates/MulticastDelegate.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#include <string_view>

// -------------------------------------------------------------
// IHotReload -- Public interface for the HotReload module.
//
// Provides DLL hot-reload orchestration: automatic file change
// detection, versioned DLL loading, and module reload sequencing.
// Matches UE's versioned DLL hot-reload pattern.
// -------------------------------------------------------------

namespace Enigma
{

class IHotReload : public IModuleInterface
{
public:
    /// Convenience accessor. Returns nullptr if module not loaded.
    static IHotReload* GetPtr()
    {
        return FModuleManager::Get().IsModuleLoaded("HotReload")
            ? &FModuleManager::Get().GetModuleChecked<IHotReload>("HotReload")
            : nullptr;
    }

    /// Manually trigger reload of a specific module.
    virtual bool ReloadModule(std::string_view ModuleName) = 0;

    /// Query whether a reload is currently in progress.
    virtual bool IsReloading() const = 0;

    /// Enable/disable automatic file monitoring.
    virtual void SetEnabled(bool bEnabled) = 0;
    virtual bool IsEnabled() const = 0;

    /// Event: broadcast after successful module reload (moduleName).
    virtual TMulticastDelegate<std::string_view>& OnModuleReloaded() = 0;

    /// Event: broadcast after failed reload (moduleName, errorMessage).
    virtual TMulticastDelegate<std::string_view, std::string_view>& OnReloadFailed() = 0;
};

} // namespace Enigma
