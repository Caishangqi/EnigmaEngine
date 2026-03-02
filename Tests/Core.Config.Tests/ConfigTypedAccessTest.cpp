// Copyright EnigmaEngine. All Rights Reserved.
// Unit tests for FConfigCacheIni typed getters/setters.

#include <gtest/gtest.h>
#include "Misc/ConfigCacheIni.h"

#include <filesystem>
#include <fstream>

using namespace Enigma;

namespace
{
    class TypedAccessFixture : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            auto base = std::filesystem::temp_directory_path() / "enigma_typed_test";
            std::filesystem::remove_all(base);
            m_engineDir = (base / "Engine" / "Config").string();
            m_projectDir = (base / "Project" / "Config").string();
            std::filesystem::create_directories(m_engineDir);
            std::filesystem::create_directories(m_projectDir);

            // Write a test config with various types.
            std::ofstream f(std::filesystem::path(m_projectDir) / "DefaultGame.ini");
            f << "[Settings]\n"
              << "Name=TestGame\n"
              << "MaxPlayers=16\n"
              << "Gravity=9.81\n"
              << "bFullscreen=true\n"
              << "bDebug=false\n"
              << "bEnabled=True\n"
              << "bDisabled=False\n"
              << "bOne=1\n"
              << "bZero=0\n"
              << "InvalidInt=abc\n"
              << "InvalidFloat=xyz\n"
              << "EmptyValue=\n";
            f.close();

            m_cache.Initialize(m_engineDir, m_projectDir);
        }

        void TearDown() override
        {
            auto base = std::filesystem::temp_directory_path() / "enigma_typed_test";
            std::filesystem::remove_all(base);
        }

        FConfigCacheIni m_cache;
        std::string m_engineDir;
        std::string m_projectDir;
    };
} // anonymous namespace

// =============================================================
// GetString
// =============================================================

TEST_F(TypedAccessFixture, GetStringSuccess)
{
    std::string val;
    EXPECT_TRUE(m_cache.GetString("Settings", "Name", val, "Game"));
    EXPECT_EQ(val, "TestGame");
}

TEST_F(TypedAccessFixture, GetStringNotFound)
{
    std::string val;
    EXPECT_FALSE(m_cache.GetString("Settings", "NonExistent", val, "Game"));
}

TEST_F(TypedAccessFixture, GetStringWrongSection)
{
    std::string val;
    EXPECT_FALSE(m_cache.GetString("WrongSection", "Name", val, "Game"));
}

TEST_F(TypedAccessFixture, GetStringWrongDomain)
{
    std::string val;
    EXPECT_FALSE(m_cache.GetString("Settings", "Name", val, "NonExistentDomain"));
}

// =============================================================
// GetInt
// =============================================================

TEST_F(TypedAccessFixture, GetIntSuccess)
{
    int32_t val = 0;
    EXPECT_TRUE(m_cache.GetInt("Settings", "MaxPlayers", val, "Game"));
    EXPECT_EQ(val, 16);
}

TEST_F(TypedAccessFixture, GetIntInvalidReturnsFalse)
{
    int32_t val = -1;
    EXPECT_FALSE(m_cache.GetInt("Settings", "InvalidInt", val, "Game"));
    EXPECT_EQ(val, -1); // Unchanged on failure
}

TEST_F(TypedAccessFixture, GetIntNotFound)
{
    int32_t val = 0;
    EXPECT_FALSE(m_cache.GetInt("Settings", "NonExistent", val, "Game"));
}

// =============================================================
// GetFloat
// =============================================================

TEST_F(TypedAccessFixture, GetFloatSuccess)
{
    float val = 0.0f;
    EXPECT_TRUE(m_cache.GetFloat("Settings", "Gravity", val, "Game"));
    EXPECT_FLOAT_EQ(val, 9.81f);
}

TEST_F(TypedAccessFixture, GetFloatInvalidReturnsFalse)
{
    float val = -1.0f;
    EXPECT_FALSE(m_cache.GetFloat("Settings", "InvalidFloat", val, "Game"));
    EXPECT_FLOAT_EQ(val, -1.0f); // Unchanged on failure
}

// =============================================================
// GetBool
// =============================================================

TEST_F(TypedAccessFixture, GetBoolTrue)
{
    bool val = false;
    EXPECT_TRUE(m_cache.GetBool("Settings", "bFullscreen", val, "Game"));
    EXPECT_TRUE(val);
}

TEST_F(TypedAccessFixture, GetBoolFalse)
{
    bool val = true;
    EXPECT_TRUE(m_cache.GetBool("Settings", "bDebug", val, "Game"));
    EXPECT_FALSE(val);
}

TEST_F(TypedAccessFixture, GetBoolTrueCapitalized)
{
    bool val = false;
    EXPECT_TRUE(m_cache.GetBool("Settings", "bEnabled", val, "Game"));
    EXPECT_TRUE(val);
}

TEST_F(TypedAccessFixture, GetBoolFalseCapitalized)
{
    bool val = true;
    EXPECT_TRUE(m_cache.GetBool("Settings", "bDisabled", val, "Game"));
    EXPECT_FALSE(val);
}

TEST_F(TypedAccessFixture, GetBoolNumericOne)
{
    bool val = false;
    EXPECT_TRUE(m_cache.GetBool("Settings", "bOne", val, "Game"));
    EXPECT_TRUE(val);
}

TEST_F(TypedAccessFixture, GetBoolNumericZero)
{
    bool val = true;
    EXPECT_TRUE(m_cache.GetBool("Settings", "bZero", val, "Game"));
    EXPECT_FALSE(val);
}

// =============================================================
// SetString / SetInt / SetFloat / SetBool
// =============================================================

TEST_F(TypedAccessFixture, SetStringAndReadBack)
{
    m_cache.SetString("Settings", "Name", "NewName", "Game");
    std::string val;
    EXPECT_TRUE(m_cache.GetString("Settings", "Name", val, "Game"));
    EXPECT_EQ(val, "NewName");
}

TEST_F(TypedAccessFixture, SetIntAndReadBack)
{
    m_cache.SetInt("Settings", "MaxPlayers", 32, "Game");
    int32_t val = 0;
    EXPECT_TRUE(m_cache.GetInt("Settings", "MaxPlayers", val, "Game"));
    EXPECT_EQ(val, 32);
}

TEST_F(TypedAccessFixture, SetFloatAndReadBack)
{
    m_cache.SetFloat("Settings", "Gravity", 10.5f, "Game");
    float val = 0.0f;
    EXPECT_TRUE(m_cache.GetFloat("Settings", "Gravity", val, "Game"));
    EXPECT_FLOAT_EQ(val, 10.5f);
}

TEST_F(TypedAccessFixture, SetBoolAndReadBack)
{
    m_cache.SetBool("Settings", "bFullscreen", false, "Game");
    bool val = true;
    EXPECT_TRUE(m_cache.GetBool("Settings", "bFullscreen", val, "Game"));
    EXPECT_FALSE(val);
}
