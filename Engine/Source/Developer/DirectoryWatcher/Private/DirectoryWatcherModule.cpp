// Copyright EnigmaEngine. All Rights Reserved.

#include "DirectoryWatcherModule.h"
#include "DirectoryWatcherProxy.h"
#include "Windows/DirectoryWatcherWindows.h"
#include "Containers/Ticker.h"
#include "Modules/ModuleMacros.h"
#include "Logging/LogMacros.h"

namespace Enigma
{

DEFINE_LOG_CATEGORY_STATIC(LogDirectoryWatcher, Info, All);

static FDelegateHandle GTickerHandle;

void FDirectoryWatcherModule::StartupModule()
{
    // Create platform watcher and wrap in proxy.
    auto* Inner = new FDirectoryWatcherWindows();
    DirectoryWatcher = new FDirectoryWatcherProxy(Inner);

    // Register with FTSTicker for per-frame tick.
    FTickerDelegate TickDelegate;
    TickDelegate.Bind([this](float DeltaTime) -> bool
    {
        if (DirectoryWatcher)
        {
            DirectoryWatcher->Tick(DeltaTime);
        }
        return true;
    });
    GTickerHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);

    ENIGMA_LOG(LogDirectoryWatcher, Info,
        "DirectoryWatcher module started (ticker registered)");
}

void FDirectoryWatcherModule::ShutdownModule()
{
    FTSTicker::RemoveTicker(GTickerHandle);

    delete DirectoryWatcher;
    DirectoryWatcher = nullptr;

    ENIGMA_LOG(LogDirectoryWatcher, Info,
        "DirectoryWatcher module shut down");
}

IDirectoryWatcher* FDirectoryWatcherModule::Get()
{
    return DirectoryWatcher;
}

void FDirectoryWatcherModule::RegisterExternalChanges(
    const std::vector<FFileChangeData>& Changes) const
{
    if (DirectoryWatcher)
    {
        DirectoryWatcher->RegisterExternalChanges(Changes);
    }
}

IMPLEMENT_MODULE(FDirectoryWatcherModule, DirectoryWatcher)

} // namespace Enigma
