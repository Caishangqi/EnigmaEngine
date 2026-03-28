// Copyright EnigmaEngine. All Rights Reserved.

#include "DirectoryWatcherProxy.h"

namespace Enigma
{

FDirectoryWatcherProxy::FDirectoryWatcherProxy(IDirectoryWatcher* InInner)
    : Inner(InInner)
{
}

FDirectoryWatcherProxy::~FDirectoryWatcherProxy()
{
    delete Inner;
    Inner = nullptr;
}

bool FDirectoryWatcherProxy::RegisterDirectoryChangedCallback(
    const std::string& Directory,
    const FDirectoryChanged& Callback,
    FDelegateHandle& OutHandle,
    uint32_t Flags)
{
    if (!Inner)
    {
        return false;
    }
    return Inner->RegisterDirectoryChangedCallback(Directory, Callback, OutHandle, Flags);
}

bool FDirectoryWatcherProxy::UnregisterDirectoryChangedCallback(
    const std::string& Directory,
    FDelegateHandle Handle)
{
    if (!Inner)
    {
        return false;
    }
    return Inner->UnregisterDirectoryChangedCallback(Directory, Handle);
}

void FDirectoryWatcherProxy::Tick(float DeltaSeconds)
{
    if (Inner)
    {
        Inner->Tick(DeltaSeconds);
    }
    ProcessPendingChanges();
}

void FDirectoryWatcherProxy::RegisterExternalChanges(
    const std::vector<FFileChangeData>& Changes)
{
    PendingExternalChanges.insert(
        PendingExternalChanges.end(),
        Changes.begin(),
        Changes.end());
}

void FDirectoryWatcherProxy::ProcessPendingChanges()
{
    if (PendingExternalChanges.empty())
    {
        return;
    }

    // Dispatch external changes through the same callback mechanism.
    // Group by directory and fire registered delegates.
    // For now, external changes are consumed and cleared.
    // The Inner watcher's registered callbacks will handle OS-detected
    // changes; external changes are an additional injection point.
    // TODO: Route external changes to matching directory callbacks
    //       if per-directory routing is needed.
    PendingExternalChanges.clear();
}

} // namespace Enigma
