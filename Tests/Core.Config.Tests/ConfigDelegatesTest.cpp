// Copyright EnigmaEngine. All Rights Reserved.
// Unit tests for FConfigDelegates event system.

#include <gtest/gtest.h>
#include "Misc/ConfigCacheIni.h"
#include "Misc/ConfigDelegates.h"

#include <filesystem>
#include <fstream>

using namespace Enigma;

namespace
{
    class DelegateFixture : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            auto base = std::filesystem::temp_directory_path() / "enigma_delegate_test";
            std::filesystem::remove_all(base);
            m_engineDir = (base / "Engine" / "Config").string();
            m_projectDir = (base / "Project" / "Config").string();
            std::filesystem::create_directories(m_engineDir);
            std::filesystem::create_directories(m_projectDir);
        }

        void TearDown() override
        {
            auto base = std::filesystem::temp_directory_path() / "enigma_delegate_test";
            std::filesystem::remove_all(base);
        }

        std::string m_engineDir;
        std::string m_projectDir;
    };
} // anonymous namespace

// =============================================================
// OnConfigLoaded fires after LoadConfigDomain
// =============================================================

TEST_F(DelegateFixture, OnConfigLoadedFiresOnInitialize)
{
    int loadCount = 0;
    std::vector<std::string> loadedDomains;

    auto handle = FConfigDelegates::OnConfigLoaded.Add(
        [&](const std::string& name)
        {
            ++loadCount;
            loadedDomains.push_back(name);
        });

    FConfigCacheIni cache;
    cache.Initialize(m_engineDir, m_projectDir);

    // Standard domains: Engine, Game, GameUserSettings, Input
    EXPECT_EQ(loadCount, 4);
    EXPECT_EQ(loadedDomains.size(), 4u);

    // Verify all standard domains were loaded.
    EXPECT_NE(std::find(loadedDomains.begin(), loadedDomains.end(), "Engine"),
              loadedDomains.end());
    EXPECT_NE(std::find(loadedDomains.begin(), loadedDomains.end(), "Game"),
              loadedDomains.end());

    FConfigDelegates::OnConfigLoaded.Remove(handle);
}

TEST_F(DelegateFixture, OnConfigLoadedFiresOnExplicitLoad)
{
    FConfigCacheIni cache;
    cache.Initialize(m_engineDir, m_projectDir);

    int loadCount = 0;
    auto handle = FConfigDelegates::OnConfigLoaded.Add(
        [&](const std::string& /*name*/) { ++loadCount; });

    cache.LoadConfigDomain("CustomDomain");
    EXPECT_EQ(loadCount, 1);

    FConfigDelegates::OnConfigLoaded.Remove(handle);
}

// =============================================================
// OnConfigSectionChanged fires after SetString
// =============================================================

TEST_F(DelegateFixture, OnConfigSectionChangedFiresOnSet)
{
    FConfigCacheIni cache;
    cache.Initialize(m_engineDir, m_projectDir);

    int changeCount = 0;
    std::string lastConfig;
    std::string lastSection;

    auto handle = FConfigDelegates::OnConfigSectionChanged.Add(
        [&](const std::string& config, const std::string& section)
        {
            ++changeCount;
            lastConfig = config;
            lastSection = section;
        });

    cache.SetString("MySection", "Key", "Value", "Engine");
    EXPECT_EQ(changeCount, 1);
    EXPECT_EQ(lastConfig, "Engine");
    EXPECT_EQ(lastSection, "MySection");

    FConfigDelegates::OnConfigSectionChanged.Remove(handle);
}

TEST_F(DelegateFixture, OnConfigSectionChangedFiresForEachSet)
{
    FConfigCacheIni cache;
    cache.Initialize(m_engineDir, m_projectDir);

    int changeCount = 0;
    auto handle = FConfigDelegates::OnConfigSectionChanged.Add(
        [&](const std::string&, const std::string&) { ++changeCount; });

    cache.SetInt("S", "A", 1, "Engine");
    cache.SetFloat("S", "B", 2.0f, "Engine");
    cache.SetBool("S", "C", true, "Engine");
    EXPECT_EQ(changeCount, 3);

    FConfigDelegates::OnConfigSectionChanged.Remove(handle);
}

// =============================================================
// OnConfigReadyForUse
// =============================================================

TEST_F(DelegateFixture, OnConfigReadyForUseBroadcasts)
{
    int readyCount = 0;
    auto handle = FConfigDelegates::OnConfigReadyForUse.Add(
        [&]() { ++readyCount; });

    // Manually broadcast (normally done by FEngineLoop::PreInit).
    FConfigDelegates::OnConfigReadyForUse.Broadcast();
    EXPECT_EQ(readyCount, 1);

    FConfigDelegates::OnConfigReadyForUse.Remove(handle);
}

// =============================================================
// Handle removal stops notifications
// =============================================================

TEST_F(DelegateFixture, RemovedHandleDoesNotFire)
{
    int count = 0;
    auto handle = FConfigDelegates::OnConfigLoaded.Add(
        [&](const std::string&) { ++count; });

    FConfigDelegates::OnConfigLoaded.Remove(handle);

    FConfigCacheIni cache;
    cache.Initialize(m_engineDir, m_projectDir);

    EXPECT_EQ(count, 0);
}

// =============================================================
// Multiple listeners
// =============================================================

TEST_F(DelegateFixture, MultipleListenersAllFire)
{
    int count1 = 0;
    int count2 = 0;

    auto h1 = FConfigDelegates::OnConfigLoaded.Add(
        [&](const std::string&) { ++count1; });
    auto h2 = FConfigDelegates::OnConfigLoaded.Add(
        [&](const std::string&) { ++count2; });

    FConfigCacheIni cache;
    cache.Initialize(m_engineDir, m_projectDir);

    EXPECT_EQ(count1, 4);
    EXPECT_EQ(count2, 4);

    FConfigDelegates::OnConfigLoaded.Remove(h1);
    FConfigDelegates::OnConfigLoaded.Remove(h2);
}
