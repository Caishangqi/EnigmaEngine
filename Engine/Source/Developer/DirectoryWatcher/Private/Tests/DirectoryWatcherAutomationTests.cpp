// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "DirectoryWatcherProxy.h"

#include <vector>

namespace Enigma
{

ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDirectoryWatcherProxyAutomationTest,
    "System.Developer.DirectoryWatcher.Proxy",
    DirectoryWatcher,
    EAutomationTestType::Integration,
    EAutomationTestFlags::None)

bool FDirectoryWatcherProxyAutomationTest::RunTest(const FAutomationTestContext& Context)
{
    FDirectoryWatcherProxy Proxy(nullptr);
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind([](const std::vector<FFileChangeData>&)
    {
    });

    FDelegateHandle Handle;
    const bool bRegistered = Proxy.RegisterDirectoryChangedCallback(
        "MissingDirectory",
        Callback,
        Handle,
        0);
    const bool bUnregistered = Proxy.UnregisterDirectoryChangedCallback(
        "MissingDirectory",
        Handle);

    TestTrue("Proxy without inner watcher should reject registration", !bRegistered);
    TestTrue("Rejected registration should not produce a valid handle", !Handle.IsValid());
    TestTrue("Proxy without inner watcher should reject unregistration", !bUnregistered);

    Proxy.RegisterExternalChanges({
        FFileChangeData
        {
            .Filename = "Generated.txt",
            .Action = FFileChangeData::FCA_Added,
        },
    });
    Proxy.Tick(0.0f);

    return !Context.HasAnyFailures();
}

} // namespace Enigma
