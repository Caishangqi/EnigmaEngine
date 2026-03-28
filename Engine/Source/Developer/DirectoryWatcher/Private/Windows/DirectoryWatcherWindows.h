// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "IDirectoryWatcher.h"
#include "Windows/DirectoryWatchRequestWindows.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// -------------------------------------------------------------
// FDirectoryWatcherWindows -- Windows implementation of
// IDirectoryWatcher using ReadDirectoryChangesW + APC.
// Matches UE's implementation pattern.
// -------------------------------------------------------------

namespace Enigma
{

class FDirectoryWatcherWindows : public IDirectoryWatcher
{
public:
    ~FDirectoryWatcherWindows() override = default;

    bool RegisterDirectoryChangedCallback(
        const std::string& Directory,
        const FDirectoryChanged& Callback,
        FDelegateHandle& OutHandle,
        uint32_t Flags) override;

    bool UnregisterDirectoryChangedCallback(
        const std::string& Directory,
        FDelegateHandle Handle) override;

    /// Calls SleepEx(0, TRUE) to drain APC queue,
    /// then ProcessPendingNotifications() on each request.
    void Tick(float DeltaSeconds) override;

private:
    /// Key: (directory, flags) pair for unique watch requests.
    using DirectoryKey = std::pair<std::string, uint32_t>;

    std::map<DirectoryKey, std::unique_ptr<FDirectoryWatchRequestWindows>> RequestMap;
    std::vector<FDirectoryWatchRequestWindows*> RequestsPendingDelete;
};

} // namespace Enigma
