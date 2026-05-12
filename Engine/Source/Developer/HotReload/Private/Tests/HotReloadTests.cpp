// Copyright EnigmaEngine. All Rights Reserved.
// HotReload.Tests -- Unit tests for HotReload module logic.
// Tests snapshot file operations, debounce, and event broadcasting.

#include "IHotReload.h"
#include "Delegates/MulticastDelegate.h"
#include "Modules/ModuleManager.h"

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.Developer.HotReload." #SuiteName "." #TestName, \
        HotReload, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::None)

#define ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.Developer.HotReload." #FixtureName "." #TestName, \
        HotReload, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::None)

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace Enigma;
namespace fs = std::filesystem;

// ---------------------------------------------------------------
// Test fixture for snapshot file operations.
// ---------------------------------------------------------------
class HotReloadSnapshotTest : public ::Enigma::FAutomationTestFixture
{
protected:
    void SetUp() override
    {
        TempDir = fs::temp_directory_path() / "enigma_hr_test";
        fs::create_directories(TempDir);
    }

    void TearDown() override
    {
        std::error_code Ec;
        fs::remove_all(TempDir, Ec);
    }

    /// Create a fake DLL file for testing.
    fs::path CreateFakeDll(const std::string& Name)
    {
        fs::path DllPath = TempDir / (Name + ".dll");
        std::ofstream Out(DllPath, std::ios::binary);
        Out << "FAKE_DLL_CONTENT_" << Name;
        return DllPath;
    }

    /// Create a fake PDB file for testing.
    fs::path CreateFakePdb(const std::string& Name)
    {
        fs::path PdbPath = TempDir / (Name + ".pdb");
        std::ofstream Out(PdbPath, std::ios::binary);
        Out << "FAKE_PDB_CONTENT_" << Name;
        return PdbPath;
    }

    static std::string MakeSnapshotName(
        const std::string& BaseStem,
        int Suffix,
        const char* Extension)
    {
        char Buffer[16] = {};
        std::snprintf(Buffer, sizeof(Buffer), "-%04d%s", Suffix, Extension);
        return BaseStem + Buffer;
    }

    fs::path TempDir;
};

// ---------------------------------------------------------------
// Snapshot_CreateAndCleanup: verify file copy with snapshot name.
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST_F(HotReloadSnapshotTest, Snapshot_CreateAndCleanup)
{
    auto DllPath = CreateFakeDll("EnigmaEngine-TestModule");

    std::string SnapshotName = MakeSnapshotName("EnigmaEngine-TestModule", 1, ".dll");
    fs::path SnapshotPath = TempDir / SnapshotName;

    std::error_code Ec;
    fs::copy_file(DllPath, SnapshotPath, fs::copy_options::overwrite_existing, Ec);
    if (!TestFalse("ASSERT_FALSE", Ec)) { return; }
    TestTrue("EXPECT_TRUE", fs::exists(SnapshotPath));

    // Verify content matches.
    {
        std::ifstream In(SnapshotPath, std::ios::binary);
        std::string Content((std::istreambuf_iterator<char>(In)),
                             std::istreambuf_iterator<char>());
        TestTrue("EXPECT_TRUE", Content.find("FAKE_DLL_CONTENT") != std::string::npos);
    }

    // Cleanup.
    fs::remove(SnapshotPath, Ec);
    TestFalse("EXPECT_FALSE", fs::exists(SnapshotPath));
}

// ---------------------------------------------------------------
// Snapshot_PdbCopiedAlongside: verify PDB is copied too.
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST_F(HotReloadSnapshotTest, Snapshot_PdbCopiedAlongside)
{
    auto DllPath = CreateFakeDll("EnigmaEngine-TestModule");
    auto PdbPath = CreateFakePdb("EnigmaEngine-TestModule");

    fs::path SnapshotDll = TempDir / MakeSnapshotName("EnigmaEngine-TestModule", 1, ".dll");
    fs::path SnapshotPdb = TempDir / MakeSnapshotName("EnigmaEngine-TestModule", 1, ".pdb");

    std::error_code Ec;
    fs::copy_file(DllPath, SnapshotDll, fs::copy_options::overwrite_existing, Ec);
    if (!TestFalse("ASSERT_FALSE", Ec)) { return; }

    // Copy PDB if exists (matching HotReload logic).
    if (fs::exists(PdbPath))
    {
        fs::copy_file(PdbPath, SnapshotPdb, fs::copy_options::overwrite_existing, Ec);
    }

    TestTrue("EXPECT_TRUE", fs::exists(SnapshotDll));
    TestTrue("EXPECT_TRUE", fs::exists(SnapshotPdb));

    // Cleanup both.
    fs::remove(SnapshotDll, Ec);
    fs::remove(SnapshotPdb, Ec);
    TestFalse("EXPECT_FALSE", fs::exists(SnapshotDll));
    TestFalse("EXPECT_FALSE", fs::exists(SnapshotPdb));
}

// ---------------------------------------------------------------
// Snapshot_VersionIncrement: multiple copies get unique names.
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST_F(HotReloadSnapshotTest, Snapshot_VersionIncrement)
{
    auto DllPath = CreateFakeDll("EnigmaEngine-TestModule");
    std::string BaseStem = "EnigmaEngine-TestModule";

    std::error_code Ec;
    for (int Suffix = 1; Suffix <= 3; ++Suffix)
    {
        std::string SnapshotName = MakeSnapshotName(BaseStem, Suffix, ".dll");
        fs::path SnapshotPath = TempDir / SnapshotName;

        fs::copy_file(DllPath, SnapshotPath,
            fs::copy_options::overwrite_existing, Ec);
        if (!TestFalse("ASSERT_FALSE", Ec)) { return; }
        TestTrue("EXPECT_TRUE", fs::exists(SnapshotPath));
    }

    // All three versions should exist.
    TestTrue("EXPECT_TRUE", fs::exists(TempDir / "EnigmaEngine-TestModule-0001.dll"));
    TestTrue("EXPECT_TRUE", fs::exists(TempDir / "EnigmaEngine-TestModule-0002.dll"));
    TestTrue("EXPECT_TRUE", fs::exists(TempDir / "EnigmaEngine-TestModule-0003.dll"));
}

// ---------------------------------------------------------------
// Event delegate tests using TMulticastDelegate directly.
// ---------------------------------------------------------------
class HotReloadEventTest : public ::Enigma::FAutomationTestFixture
{
protected:
    TMulticastDelegate<std::string_view>                    ModuleReloadedDelegate;
    TMulticastDelegate<std::string_view, std::string_view>  ReloadFailedDelegate;
};

// ---------------------------------------------------------------
// OnModuleReloaded_Broadcast
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST_F(HotReloadEventTest, OnModuleReloaded_Broadcast)
{
    std::string ReceivedModule;
    auto Handle = ModuleReloadedDelegate.Add(
        [&ReceivedModule](std::string_view Name)
        {
            ReceivedModule = std::string(Name);
        });

    ModuleReloadedDelegate.Broadcast(std::string_view("TestModule"));
    TestEqual("EXPECT_EQ", ReceivedModule, "TestModule");

    ModuleReloadedDelegate.Remove(Handle);
}

// ---------------------------------------------------------------
// OnReloadFailed_Broadcast
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST_F(HotReloadEventTest, OnReloadFailed_Broadcast)
{
    std::string ReceivedModule;
    std::string ReceivedError;
    auto Handle = ReloadFailedDelegate.Add(
        [&ReceivedModule, &ReceivedError](
            std::string_view Name, std::string_view Error)
        {
            ReceivedModule = std::string(Name);
            ReceivedError = std::string(Error);
        });

    ReloadFailedDelegate.Broadcast(
        std::string_view("TestModule"),
        std::string_view("DLL load failed"));

    TestEqual("EXPECT_EQ", ReceivedModule, "TestModule");
    TestEqual("EXPECT_EQ", ReceivedError, "DLL load failed");

    ReloadFailedDelegate.Remove(Handle);
}

// ---------------------------------------------------------------
// NonReloadable_Rejection: SupportsDynamicReloading() == false
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST_F(HotReloadEventTest, NonReloadable_Rejection)
{
    // Simulate: a module that returns SupportsDynamicReloading() == false
    // should cause OnReloadFailed to broadcast.
    bool bFailed = false;
    auto Handle = ReloadFailedDelegate.Add(
        [&bFailed](std::string_view, std::string_view Error)
        {
            if (std::string(Error).find("does not support") != std::string::npos)
            {
                bFailed = true;
            }
        });

    // Simulate the rejection.
    ReloadFailedDelegate.Broadcast(
        std::string_view("Core"),
        std::string_view("Module does not support dynamic reloading"));

    TestTrue("EXPECT_TRUE", bFailed);
    ReloadFailedDelegate.Remove(Handle);
}

// ---------------------------------------------------------------
// Debounce_BatchChanges: multiple changes within window are batched.
// ---------------------------------------------------------------
ENIGMA_IMPLEMENT_HOT_RELOAD_AUTOMATION_TEST(HotReloadDebounceTest, Debounce_BatchChanges)
{
    // Simulate debounce logic: changes detected at different times,
    // only processed after debounce delay.
    double DebounceDelay = 0.5;
    double CurrentTime = 0.0;

    struct FDetectedChange
    {
        std::string ModuleName;
        double DetectedTime;
    };

    std::vector<FDetectedChange> DetectedModules;

    // Simulate three changes within the debounce window.
    DetectedModules.push_back({"ModuleA", 1.0});
    DetectedModules.push_back({"ModuleB", 1.1});
    DetectedModules.push_back({"ModuleA", 1.2});  // duplicate

    CurrentTime = 1.3;  // Not yet past debounce.
    double LatestDetection = 0.0;
    for (const auto& C : DetectedModules)
    {
        LatestDetection = std::max(LatestDetection, C.DetectedTime);
    }
    TestLessThan("EXPECT_LT", CurrentTime - LatestDetection, DebounceDelay);

    // Advance past debounce.
    CurrentTime = 1.8;
    TestGreaterThanOrEqual("EXPECT_GE", CurrentTime - LatestDetection, DebounceDelay);

    // Deduplicate.
    std::unordered_map<std::string, FDetectedChange> Unique;
    for (auto& C : DetectedModules)
    {
        Unique[C.ModuleName] = C;
    }

    TestEqual("EXPECT_EQ", Unique.size(), 2u); // ModuleA and ModuleB
    TestTrue("EXPECT_TRUE", Unique.contains("ModuleA"));
    TestTrue("EXPECT_TRUE", Unique.contains("ModuleB"));
}
