// Copyright EnigmaEngine. All Rights Reserved.

#include "Windows/DirectoryWatcherWindows.h"

#include <algorithm>

namespace Enigma
{

bool FDirectoryWatcherWindows::RegisterDirectoryChangedCallback(
    const std::string& Directory,
    const FDirectoryChanged& Callback,
    FDelegateHandle& OutHandle,
    uint32_t Flags)
{
    DirectoryKey Key{Directory, Flags};

    auto It = RequestMap.find(Key);
    if (It == RequestMap.end())
    {
        // Create a new watch request for this directory+flags.
        auto Request = std::make_unique<FDirectoryWatchRequestWindows>(Flags);
        if (!Request->Init(Directory))
        {
            return false;
        }

        auto [InsertIt, _] = RequestMap.emplace(Key, std::move(Request));
        It = InsertIt;
    }

    OutHandle = It->second->AddDelegate(Callback);
    return true;
}

bool FDirectoryWatcherWindows::UnregisterDirectoryChangedCallback(
    const std::string& Directory,
    FDelegateHandle Handle)
{
    // Search all flag variants for this directory.
    for (auto It = RequestMap.begin(); It != RequestMap.end(); ++It)
    {
        if (It->first.first == Directory)
        {
            if (It->second->RemoveDelegate(Handle))
            {
                // If no delegates remain, schedule for deletion.
                if (!It->second->HasDelegates())
                {
                    It->second->EndWatchRequest();
                    RequestsPendingDelete.push_back(It->second.get());
                }
                return true;
            }
        }
    }
    return false;
}

void FDirectoryWatcherWindows::Tick(float DeltaSeconds)
{
    // Drain the APC queue (alertable wait, zero timeout).
    ::SleepEx(0, TRUE);

    // Process pending notifications on active requests.
    for (auto& [Key, Request] : RequestMap)
    {
        if (!Request->IsPendingDelete())
        {
            Request->ProcessPendingNotifications();
        }
    }

    // Clean up requests pending deletion.
    for (auto* PendingRequest : RequestsPendingDelete)
    {
        for (auto It = RequestMap.begin(); It != RequestMap.end(); ++It)
        {
            if (It->second.get() == PendingRequest)
            {
                RequestMap.erase(It);
                break;
            }
        }
    }
    RequestsPendingDelete.clear();
}

} // namespace Enigma
