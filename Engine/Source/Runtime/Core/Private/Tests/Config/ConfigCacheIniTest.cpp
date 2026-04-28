// Copyright EnigmaEngine. All Rights Reserved.
// Unit tests for FConfigCacheIni layered config cache.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(SuiteName, TestName)                                               \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST(                                      \
		F##SuiteName##_##TestName##AutomationTest,                                        \
		"System.Core.Config." #SuiteName "." #TestName,                                      \
		Core,                                                                    \
		::Enigma::EAutomationTestType::Unit,                                                       \
		::Enigma::EAutomationTestFlags::None)

#define ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST_F(FixtureName, TestName)                                      \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST_F(                                    \
		FixtureName,                                                                     \
		F##FixtureName##_##TestName##AutomationTest,                                     \
		"System.Core.Config." #FixtureName "." #TestName,                                    \
		Core,                                                                    \
		::Enigma::EAutomationTestType::Unit,                                                       \
		::Enigma::EAutomationTestFlags::None)
#include "Misc/ConfigCacheIni.h"

#include <filesystem>
#include <fstream>

using namespace Enigma;

namespace
{
    /// Helper: create a temp directory and return its path.
    class TempConfigDir
    {
    public:
        TempConfigDir()
        {
            m_path = std::filesystem::temp_directory_path() / "enigma_config_test";
            // Clean up any previous run.
            std::filesystem::remove_all(m_path);
            std::filesystem::create_directories(m_path);
        }

        ~TempConfigDir()
        {
            std::filesystem::remove_all(m_path);
        }

        std::string Path() const { return m_path.string(); }

        void WriteFile(const std::string& filename, const std::string& content)
        {
            std::ofstream f(m_path / filename);
            f << content;
        }

    private:
        std::filesystem::path m_path;
    };

    class TempEngineProjectDirs
    {
    public:
        TempEngineProjectDirs()
        {
            auto base = std::filesystem::temp_directory_path() / "enigma_cache_test";
            std::filesystem::remove_all(base);
            m_engineDir = (base / "Engine" / "Config").string();
            m_projectDir = (base / "Project" / "Config").string();
            std::filesystem::create_directories(m_engineDir);
            std::filesystem::create_directories(m_projectDir);
        }

        ~TempEngineProjectDirs()
        {
            auto base = std::filesystem::temp_directory_path() / "enigma_cache_test";
            std::filesystem::remove_all(base);
        }

        const std::string& EngineDir() const { return m_engineDir; }
        const std::string& ProjectDir() const { return m_projectDir; }

        void WriteEngineFile(const std::string& filename, const std::string& content)
        {
            std::ofstream f(std::filesystem::path(m_engineDir) / filename);
            f << content;
        }

        void WriteProjectFile(const std::string& filename, const std::string& content)
        {
            std::ofstream f(std::filesystem::path(m_projectDir) / filename);
            f << content;
        }

    private:
        std::string m_engineDir;
        std::string m_projectDir;
    };
} // anonymous namespace

// =============================================================
// Basic initialization
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, InitializeWithEmptyDirs)
{
    FConfigCacheIni cache;
    // Should not crash even with non-existent dirs.
    cache.Initialize("/nonexistent/engine", "/nonexistent/project");
}

// =============================================================
// Single layer loading
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, LoadEngineBaseLayer)
{
    TempEngineProjectDirs dirs;
    dirs.WriteEngineFile("BaseEngine.ini",
        "[/Script/Engine.Settings]\n"
        "MaxFPS=60\n"
        "bVSync=true\n"
    );

    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    std::string maxFps;
    TestTrue("EXPECT_TRUE", cache.GetString("/Script/Engine.Settings", "MaxFPS", maxFps, "Engine"));
    TestEqual("EXPECT_EQ", maxFps, "60");

    std::string vsync;
    TestTrue("EXPECT_TRUE", cache.GetString("/Script/Engine.Settings", "bVSync", vsync, "Engine"));
    TestEqual("EXPECT_EQ", vsync, "true");
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, LoadProjectDefaultLayer)
{
    TempEngineProjectDirs dirs;
    dirs.WriteProjectFile("DefaultEngine.ini",
        "[/Script/Engine.Settings]\n"
        "GameTitle=TestGame\n"
    );

    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    std::string title;
    TestTrue("EXPECT_TRUE", cache.GetString("/Script/Engine.Settings", "GameTitle", title, "Engine"));
    TestEqual("EXPECT_EQ", title, "TestGame");
}

// =============================================================
// Multi-layer merge (project overrides engine)
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, ProjectOverridesEngine)
{
    TempEngineProjectDirs dirs;
    dirs.WriteEngineFile("BaseEngine.ini",
        "[Settings]\n"
        "MaxFPS=60\n"
        "Quality=Low\n"
    );
    dirs.WriteProjectFile("DefaultEngine.ini",
        "[Settings]\n"
        "MaxFPS=120\n"
    );

    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    std::string maxFps;
    TestTrue("EXPECT_TRUE", cache.GetString("Settings", "MaxFPS", maxFps, "Engine"));
    TestEqual("EXPECT_EQ", maxFps, "120"); // Project overrides engine

    std::string quality;
    TestTrue("EXPECT_TRUE", cache.GetString("Settings", "Quality", quality, "Engine"));
    TestEqual("EXPECT_EQ", quality, "Low"); // Engine value preserved
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, UserOverridesProject)
{
    TempEngineProjectDirs dirs;
    dirs.WriteProjectFile("DefaultEngine.ini",
        "[Settings]\nMaxFPS=120\n"
    );
    dirs.WriteProjectFile("UserEngine.ini",
        "[Settings]\nMaxFPS=30\n"
    );

    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    std::string maxFps;
    TestTrue("EXPECT_TRUE", cache.GetString("Settings", "MaxFPS", maxFps, "Engine"));
    TestEqual("EXPECT_EQ", maxFps, "30"); // User overrides project
}

// =============================================================
// Array merge across layers
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, ArrayMergeAcrossLayers)
{
    TempEngineProjectDirs dirs;
    dirs.WriteEngineFile("BaseEngine.ini",
        "[Plugins]\n+ActivePlugins=Core\n+ActivePlugins=Renderer\n"
    );
    dirs.WriteProjectFile("DefaultEngine.ini",
        "[Plugins]\n+ActivePlugins=MyPlugin\n"
    );

    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    std::vector<std::string> plugins;
    int32_t count = cache.GetArray("Plugins", "ActivePlugins", plugins, "Engine");
    TestGreaterThanOrEqual("EXPECT_GE", count, 3);
}

// =============================================================
// Missing layers (graceful handling)
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, MissingLayersDoNotCrash)
{
    TempEngineProjectDirs dirs;
    // No files written -- all layers missing.
    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    std::string val;
    TestFalse("EXPECT_FALSE", cache.GetString("Any", "Key", val, "Engine"));
}

// =============================================================
// GetSection
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, GetSectionReturnsNullForMissing)
{
    FConfigCacheIni cache;
    cache.Initialize("/nonexistent", "/nonexistent");

    TestEqual("EXPECT_EQ", cache.GetSection("NoSection", "Engine"), nullptr);
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, GetSectionReturnsValidPointer)
{
    TempEngineProjectDirs dirs;
    dirs.WriteEngineFile("BaseEngine.ini", "[MySection]\nKey=Val\n");

    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    const FConfigSection* sec = cache.GetSection("MySection", "Engine");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }
    TestNotEqual("EXPECT_NE", sec->GetValueString("Key"), nullptr);
}

// =============================================================
// Multiple domains
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigCacheIniTest, MultipleDomains)
{
    TempEngineProjectDirs dirs;
    dirs.WriteEngineFile("BaseEngine.ini", "[E]\nA=1\n");
    dirs.WriteEngineFile("BaseGame.ini", "[G]\nB=2\n");

    FConfigCacheIni cache;
    cache.Initialize(dirs.EngineDir(), dirs.ProjectDir());

    std::string a, b;
    TestTrue("EXPECT_TRUE", cache.GetString("E", "A", a, "Engine"));
    TestTrue("EXPECT_TRUE", cache.GetString("G", "B", b, "Game"));
    TestEqual("EXPECT_EQ", a, "1");
    TestEqual("EXPECT_EQ", b, "2");
}
