// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "IDirectoryWatcher.h"

#include <vector>

// -------------------------------------------------------------
// FDirectoryWatcherProxy -- Decorator wrapping the platform-
// specific watcher (matching UE's FDirectoryWatcherProxy).
// Provides external change injection and path-based filtering.
// -------------------------------------------------------------

namespace Enigma
{

class FDirectoryWatcherProxy : public IDirectoryWatcher
{
public:
    /// Takes ownership of InInner.
    explicit FDirectoryWatcherProxy(IDirectoryWatcher* InInner);
    ~FDirectoryWatcherProxy();

    bool RegisterDirectoryChangedCallback(
        const std::string& Directory,
        const FDirectoryChanged& Callback,
        FDelegateHandle& OutHandle,
        uint32_t Flags) override;

    bool UnregisterDirectoryChangedCallback(
        const std::string& Directory,
        FDelegateHandle Handle) override;

    /// Ticks the inner platform watcher, then processes pending
    /// external changes.
    void Tick(float DeltaSeconds) override;

    /// Inject external file changes (not detected by OS).
    void RegisterExternalChanges(const std::vector<FFileChangeData>& Changes);

private:
    void ProcessPendingChanges();

    IDirectoryWatcher*              Inner = nullptr;
    std::vector<FFileChangeData>    PendingExternalChanges;
};

} // namespace Enigma
