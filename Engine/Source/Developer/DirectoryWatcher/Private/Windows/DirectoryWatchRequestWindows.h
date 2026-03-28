// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "IDirectoryWatcher.h"
#include "Delegates/DelegateHandle.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

// -------------------------------------------------------------
// FDirectoryWatchRequestWindows -- Per-directory watch request
// using ReadDirectoryChangesW + APC completion routine.
// Matches UE's implementation pattern.
// -------------------------------------------------------------

namespace Enigma
{

class FDirectoryWatchRequestWindows
{
public:
    explicit FDirectoryWatchRequestWindows(uint32_t InFlags);
    ~FDirectoryWatchRequestWindows();

    FDirectoryWatchRequestWindows(const FDirectoryWatchRequestWindows&) = delete;
    FDirectoryWatchRequestWindows& operator=(const FDirectoryWatchRequestWindows&) = delete;

    /// Initialize the watch on the given directory.
    bool Init(const std::string& InDirectory);

    /// Add a delegate to receive change notifications.
    FDelegateHandle AddDelegate(const IDirectoryWatcher::FDirectoryChanged& Callback);

    /// Remove a delegate by handle.
    bool RemoveDelegate(FDelegateHandle Handle);

    /// Check if any delegates are still registered.
    bool HasDelegates() const;

    /// Cancel the watch request (async).
    void EndWatchRequest();

    /// True if this request is pending deletion.
    bool IsPendingDelete() const { return bPendingDelete; }

    /// Dispatch accumulated file changes to registered delegates.
    void ProcessPendingNotifications();

private:
    /// Process a completed ReadDirectoryChangesW result.
    void ProcessChange(DWORD InError, DWORD InNumBytes);

    /// APC completion routine called by the OS.
    static void CALLBACK ChangeNotification(
        DWORD InError, DWORD InNumBytes, LPOVERLAPPED InOverlapped);

    std::string     Directory;
    HANDLE          DirectoryHandle = INVALID_HANDLE_VALUE;
    uint32_t        NotifyFilter = 0;
    bool            bWatchSubtree = true;
    bool            bPendingDelete = false;
    bool            bEndWatchRequestInvoked = false;

    static constexpr size_t BufferSize = 16384;
    std::vector<uint8_t>    Buffer;
    std::vector<uint8_t>    BackBuffer;
    OVERLAPPED              Overlapped{};

    struct FDelegateEntry
    {
        FDelegateHandle                     Handle;
        IDirectoryWatcher::FDirectoryChanged Delegate;
    };
    std::vector<FDelegateEntry>     Delegates;
    std::vector<FFileChangeData>    FileChanges;
};

} // namespace Enigma
