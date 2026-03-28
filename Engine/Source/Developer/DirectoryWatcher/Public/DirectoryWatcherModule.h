// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "IDirectoryWatcher.h"
#include "DirectoryWatcherAPI.generated.h"
#include "Modules/ModuleInterface.h"

#include <vector>

// -------------------------------------------------------------
// FDirectoryWatcherModule -- Module class for DirectoryWatcher.
//
// Provides access to the platform-specific directory watcher
// via a proxy layer. Registers with FTSTicker for per-frame
// tick. Matches UE's FDirectoryWatcherModule structure.
// -------------------------------------------------------------

namespace Enigma
{

// Forward declaration (private implementation).
class FDirectoryWatcherProxy;

/// Module class providing access to the platform watcher via proxy.
class DIRECTORYWATCHER_API FDirectoryWatcherModule : public IModuleInterface
{
public:
    void StartupModule() override;
    void ShutdownModule() override;
    bool SupportsDynamicReloading() override { return false; }

    /// Returns the proxy watcher, or nullptr if unsupported.
    IDirectoryWatcher* Get();

    /// Register external changes not detected by OS watcher.
    void RegisterExternalChanges(const std::vector<FFileChangeData>& Changes) const;

private:
    FDirectoryWatcherProxy* DirectoryWatcher = nullptr;
};

} // namespace Enigma
