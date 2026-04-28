// Copyright EnigmaEngine. All Rights Reserved.
// DirectoryWatcher.Tests -- Unit tests for DirectoryWatcher module.

#include "IDirectoryWatcher.h"
#include "Windows/DirectoryWatcherWindows.h"

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.Developer.DirectoryWatcher.Windows." #SuiteName "." #TestName, \
        DirectoryWatcher, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::None)

#define ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.Developer.DirectoryWatcher.Windows." #FixtureName "." #TestName, \
        DirectoryWatcher, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::None)

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace Enigma;
namespace fs = std::filesystem;

// ---------------------------------------------------------------
// Test fixture: creates a temp directory, cleans up after.
// ---------------------------------------------------------------
class DirectoryWatcherTest : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        TempDir = fs::temp_directory_path() / "enigma_dw_test";
        fs::create_directories(TempDir);
        Watcher = std::make_unique<FDirectoryWatcherWindows>();
    }

    void TearDown() override
    {
        Watcher.reset();
        std::error_code Ec;
        fs::remove_all(TempDir, Ec);
    }

    /// Tick the watcher multiple times with small sleeps to allow
    /// APC callbacks to fire.
    void TickWatcher(int Count = 10, int SleepMs = 50)
    {
        for (int i = 0; i < Count; ++i)
        {
            Watcher->Tick(0.016f);
            std::this_thread::sleep_for(
                std::chrono::milliseconds(SleepMs));
        }
    }

    fs::path TempDir;
    std::unique_ptr<FDirectoryWatcherWindows> Watcher;
};

// ---------------------------------------------------------------
// RegisterCallback_Success
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(DirectoryWatcherTest, RegisterCallback_Success)
{
    std::vector<FFileChangeData> Received;
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind([&Received](const std::vector<FFileChangeData>& Changes)
    {
        Received.insert(Received.end(), Changes.begin(), Changes.end());
    });

    FDelegateHandle Handle;
    bool bOk = Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), Callback, Handle, 0);

    TestTrue("EXPECT_TRUE", bOk);
    TestTrue("EXPECT_TRUE", Handle.IsValid());
}

// ---------------------------------------------------------------
// UnregisterCallback_Success
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(DirectoryWatcherTest, UnregisterCallback_Success)
{
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind([](const std::vector<FFileChangeData>&) {});

    FDelegateHandle Handle;
    Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), Callback, Handle, 0);

    bool bRemoved = Watcher->UnregisterDirectoryChangedCallback(
        TempDir.string(), Handle);
    TestTrue("EXPECT_TRUE", bRemoved);
}

// ---------------------------------------------------------------
// DetectFileAdd
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(DirectoryWatcherTest, DetectFileAdd)
{
    std::vector<FFileChangeData> Received;
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind([&Received](const std::vector<FFileChangeData>& Changes)
    {
        Received.insert(Received.end(), Changes.begin(), Changes.end());
    });

    FDelegateHandle Handle;
    Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), Callback, Handle, 0);

    // Create a file.
    {
        std::ofstream Out(TempDir / "test_add.txt");
        Out << "hello";
    }

    TickWatcher();

    if (!TestFalse("ASSERT_FALSE", Received.empty())) { return; }
    bool FoundAdd = false;
    for (const auto& Change : Received)
    {
        if (Change.Filename.find("test_add.txt") != std::string::npos
            && Change.Action == FFileChangeData::FCA_Added)
        {
            FoundAdd = true;
            break;
        }
    }
    TestTrue("EXPECT_TRUE", FoundAdd);
}

// ---------------------------------------------------------------
// DetectFileModify
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(DirectoryWatcherTest, DetectFileModify)
{
    // Pre-create the file.
    fs::path FilePath = TempDir / "test_modify.txt";
    {
        std::ofstream Out(FilePath);
        Out << "initial";
    }

    // Wait a bit, then register watcher.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<FFileChangeData> Received;
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind([&Received](const std::vector<FFileChangeData>& Changes)
    {
        Received.insert(Received.end(), Changes.begin(), Changes.end());
    });

    FDelegateHandle Handle;
    Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), Callback, Handle, 0);

    // Modify the file.
    {
        std::ofstream Out(FilePath, std::ios::app);
        Out << " modified";
    }

    TickWatcher();

    if (!TestFalse("ASSERT_FALSE", Received.empty())) { return; }
    bool FoundModify = false;
    for (const auto& Change : Received)
    {
        if (Change.Filename.find("test_modify.txt") != std::string::npos
            && Change.Action == FFileChangeData::FCA_Modified)
        {
            FoundModify = true;
            break;
        }
    }
    TestTrue("EXPECT_TRUE", FoundModify);
}

// ---------------------------------------------------------------
// DetectFileRemove
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(DirectoryWatcherTest, DetectFileRemove)
{
    // Pre-create the file.
    fs::path FilePath = TempDir / "test_remove.txt";
    {
        std::ofstream Out(FilePath);
        Out << "to be removed";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<FFileChangeData> Received;
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind([&Received](const std::vector<FFileChangeData>& Changes)
    {
        Received.insert(Received.end(), Changes.begin(), Changes.end());
    });

    FDelegateHandle Handle;
    Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), Callback, Handle, 0);

    // Delete the file.
    fs::remove(FilePath);

    TickWatcher();

    if (!TestFalse("ASSERT_FALSE", Received.empty())) { return; }
    bool FoundRemove = false;
    for (const auto& Change : Received)
    {
        if (Change.Filename.find("test_remove.txt") != std::string::npos
            && Change.Action == FFileChangeData::FCA_Removed)
        {
            FoundRemove = true;
            break;
        }
    }
    TestTrue("EXPECT_TRUE", FoundRemove);
}

// ---------------------------------------------------------------
// MultipleWatchers_SameDirectory
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(DirectoryWatcherTest, MultipleWatchers_SameDirectory)
{
    std::vector<FFileChangeData> ReceivedA;
    std::vector<FFileChangeData> ReceivedB;

    IDirectoryWatcher::FDirectoryChanged CallbackA;
    CallbackA.Bind([&ReceivedA](const std::vector<FFileChangeData>& Changes)
    {
        ReceivedA.insert(ReceivedA.end(), Changes.begin(), Changes.end());
    });

    IDirectoryWatcher::FDirectoryChanged CallbackB;
    CallbackB.Bind([&ReceivedB](const std::vector<FFileChangeData>& Changes)
    {
        ReceivedB.insert(ReceivedB.end(), Changes.begin(), Changes.end());
    });

    FDelegateHandle HandleA, HandleB;
    Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), CallbackA, HandleA, 0);
    Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), CallbackB, HandleB, 0);

    // Create a file.
    {
        std::ofstream Out(TempDir / "test_multi.txt");
        Out << "multi";
    }

    TickWatcher();

    TestFalse("EXPECT_FALSE", ReceivedA.empty());
    TestFalse("EXPECT_FALSE", ReceivedB.empty());
}

// ---------------------------------------------------------------
// UnregisterCallback_NoMoreNotifications
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_DIRECTORY_WATCHER_WINDOWS_AUTOMATION_TEST_F(DirectoryWatcherTest, UnregisterCallback_NoMoreNotifications)
{
    std::vector<FFileChangeData> Received;
    IDirectoryWatcher::FDirectoryChanged Callback;
    Callback.Bind([&Received](const std::vector<FFileChangeData>& Changes)
    {
        Received.insert(Received.end(), Changes.begin(), Changes.end());
    });

    FDelegateHandle Handle;
    Watcher->RegisterDirectoryChangedCallback(
        TempDir.string(), Callback, Handle, 0);

    // Unregister immediately.
    Watcher->UnregisterDirectoryChangedCallback(TempDir.string(), Handle);

    // Create a file.
    {
        std::ofstream Out(TempDir / "test_unreg.txt");
        Out << "should not be seen";
    }

    TickWatcher();

    // Should not have received anything after unregister.
    TestTrue("EXPECT_TRUE", Received.empty());
}
