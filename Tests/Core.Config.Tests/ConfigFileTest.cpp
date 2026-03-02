// Copyright EnigmaEngine. All Rights Reserved.
// Unit tests for FConfigFile INI parser.

#include <gtest/gtest.h>
#include "Misc/ConfigCacheIni.h"

using namespace Enigma;

// =============================================================
// FConfigFile::ReadFromString — basic parsing
// =============================================================

TEST(ConfigFileTest, ParseEmptyString)
{
    FConfigFile config;
    EXPECT_TRUE(config.ReadFromString(""));
    EXPECT_TRUE(config.IsEmpty());
}

TEST(ConfigFileTest, ParseSingleSection)
{
    FConfigFile config;
    config.ReadFromString("[MySection]\nKey=Value\n");

    EXPECT_TRUE(config.ContainsSection("MySection"));
    const FConfigSection* sec = config.FindSection("MySection");
    ASSERT_NE(sec, nullptr);

    const std::string* val = sec->GetValueString("Key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, "Value");
}

TEST(ConfigFileTest, ParseMultipleSections)
{
    FConfigFile config;
    config.ReadFromString(
        "[Section1]\nA=1\n"
        "[Section2]\nB=2\n"
    );

    EXPECT_TRUE(config.ContainsSection("Section1"));
    EXPECT_TRUE(config.ContainsSection("Section2"));

    auto names = config.GetSectionNames();
    EXPECT_EQ(names.size(), 2u);
}

TEST(ConfigFileTest, ParseMultipleKeysInSection)
{
    FConfigFile config;
    config.ReadFromString("[S]\nA=1\nB=2\nC=3\n");

    const FConfigSection* sec = config.FindSection("S");
    ASSERT_NE(sec, nullptr);

    const std::string* a = sec->GetValueString("A");
    const std::string* b = sec->GetValueString("B");
    const std::string* c = sec->GetValueString("C");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(*a, "1");
    EXPECT_EQ(*b, "2");
    EXPECT_EQ(*c, "3");
}

// =============================================================
// Comments and blank lines
// =============================================================

TEST(ConfigFileTest, SkipComments)
{
    FConfigFile config;
    config.ReadFromString(
        "; This is a comment\n"
        "# This is also a comment\n"
        "[Section]\n"
        "; inline comment line\n"
        "Key=Value\n"
    );

    const FConfigSection* sec = config.FindSection("Section");
    ASSERT_NE(sec, nullptr);
    EXPECT_NE(sec->GetValueString("Key"), nullptr);
}

TEST(ConfigFileTest, SkipBlankLines)
{
    FConfigFile config;
    config.ReadFromString(
        "\n\n[Section]\n\nKey=Value\n\n"
    );

    const FConfigSection* sec = config.FindSection("Section");
    ASSERT_NE(sec, nullptr);
    EXPECT_NE(sec->GetValueString("Key"), nullptr);
}

// =============================================================
// Array operators
// =============================================================

TEST(ConfigFileTest, ArrayAddUniqueOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n+Items=Apple\n+Items=Banana\n");

    const FConfigSection* sec = config.FindSection("S");
    ASSERT_NE(sec, nullptr);

    auto values = sec->GetValues("Items");
    EXPECT_EQ(values.size(), 2u);
    EXPECT_EQ(values[0]->Value, "Apple");
    EXPECT_EQ(values[1]->Value, "Banana");
    EXPECT_EQ(values[0]->ValueType, EConfigValueType::ArrayAddUnique);
}

TEST(ConfigFileTest, ArrayAddOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n.Items=Apple\n.Items=Apple\n");

    const FConfigSection* sec = config.FindSection("S");
    ASSERT_NE(sec, nullptr);

    auto values = sec->GetValues("Items");
    EXPECT_EQ(values.size(), 2u);
    EXPECT_EQ(values[0]->ValueType, EConfigValueType::ArrayAdd);
}

TEST(ConfigFileTest, RemoveOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n-Items=Apple\n");

    const FConfigSection* sec = config.FindSection("S");
    ASSERT_NE(sec, nullptr);

    auto values = sec->GetValues("Items");
    EXPECT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0]->ValueType, EConfigValueType::Remove);
}

TEST(ConfigFileTest, ClearOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n!Items=ClearArray\n");

    const FConfigSection* sec = config.FindSection("S");
    ASSERT_NE(sec, nullptr);

    auto values = sec->GetValues("Items");
    EXPECT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0]->ValueType, EConfigValueType::Clear);
}

// =============================================================
// Whitespace trimming
// =============================================================

TEST(ConfigFileTest, TrimWhitespace)
{
    FConfigFile config;
    config.ReadFromString("  [ Section ]  \n  Key  =  Value  \n");

    EXPECT_TRUE(config.ContainsSection("Section"));
    const FConfigSection* sec = config.FindSection("Section");
    ASSERT_NE(sec, nullptr);

    const std::string* val = sec->GetValueString("Key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, "Value");
}

// =============================================================
// Malformed input recovery
// =============================================================

TEST(ConfigFileTest, MalformedSectionHeader)
{
    FConfigFile config;
    // Missing closing bracket -- should log warning but not crash.
    config.ReadFromString("[Broken\nKey=Value\n[Good]\nA=B\n");

    // "Broken" section may or may not be created depending on impl.
    // "Good" section should exist.
    EXPECT_TRUE(config.ContainsSection("Good"));
}

TEST(ConfigFileTest, LineWithoutEquals)
{
    FConfigFile config;
    // No '=' sign -- should log warning and skip.
    config.ReadFromString("[S]\nNoEqualsHere\nKey=Value\n");

    const FConfigSection* sec = config.FindSection("S");
    ASSERT_NE(sec, nullptr);

    const std::string* val = sec->GetValueString("Key");
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, "Value");
}

TEST(ConfigFileTest, KeyValueBeforeAnySection)
{
    FConfigFile config;
    // Key before any section header -- should log warning and skip.
    config.ReadFromString("Orphan=Value\n[S]\nKey=Value\n");

    EXPECT_TRUE(config.ContainsSection("S"));
}

// =============================================================
// Round-trip: ReadFromString -> WriteToString -> ReadFromString
// =============================================================

TEST(ConfigFileTest, RoundTrip)
{
    const std::string original =
        "[Section1]\n"
        "A=1\n"
        "B=2\n"
        "\n"
        "[Section2]\n"
        "C=3\n";

    FConfigFile config1;
    config1.ReadFromString(original);

    std::string serialized = config1.WriteToString();

    FConfigFile config2;
    config2.ReadFromString(serialized);

    EXPECT_EQ(config1, config2);
}

// =============================================================
// File I/O (non-existent file)
// =============================================================

TEST(ConfigFileTest, ReadNonExistentFile)
{
    FConfigFile config;
    EXPECT_FALSE(config.Read("__nonexistent_path_12345__.ini"));
}

// =============================================================
// FindOrAddSection
// =============================================================

TEST(ConfigFileTest, FindOrAddSection)
{
    FConfigFile config;
    EXPECT_TRUE(config.IsEmpty());

    FConfigSection* sec = config.FindOrAddSection("NewSection");
    ASSERT_NE(sec, nullptr);
    EXPECT_TRUE(config.ContainsSection("NewSection"));
    EXPECT_FALSE(config.IsEmpty());
}
