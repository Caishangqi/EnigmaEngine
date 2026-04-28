// Copyright EnigmaEngine. All Rights Reserved.
// Unit tests for FConfigFile INI parser.

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

using namespace Enigma;

// =============================================================
// FConfigFile::ReadFromString ??basic parsing
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ParseEmptyString)
{
    FConfigFile config;
    TestTrue("EXPECT_TRUE", config.ReadFromString(""));
    TestTrue("EXPECT_TRUE", config.IsEmpty());
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ParseSingleSection)
{
    FConfigFile config;
    config.ReadFromString("[MySection]\nKey=Value\n");

    TestTrue("EXPECT_TRUE", config.ContainsSection("MySection"));
    const FConfigSection* sec = config.FindSection("MySection");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    const std::string* val = sec->GetValueString("Key");
    if (!TestNotEqual("ASSERT_NE", val, nullptr)) { return; }
    TestEqual("EXPECT_EQ", *val, "Value");
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ParseMultipleSections)
{
    FConfigFile config;
    config.ReadFromString(
        "[Section1]\nA=1\n"
        "[Section2]\nB=2\n"
    );

    TestTrue("EXPECT_TRUE", config.ContainsSection("Section1"));
    TestTrue("EXPECT_TRUE", config.ContainsSection("Section2"));

    auto names = config.GetSectionNames();
    TestEqual("EXPECT_EQ", names.size(), 2u);
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ParseMultipleKeysInSection)
{
    FConfigFile config;
    config.ReadFromString("[S]\nA=1\nB=2\nC=3\n");

    const FConfigSection* sec = config.FindSection("S");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    const std::string* a = sec->GetValueString("A");
    const std::string* b = sec->GetValueString("B");
    const std::string* c = sec->GetValueString("C");
    if (!TestNotEqual("ASSERT_NE", a, nullptr)) { return; }
    if (!TestNotEqual("ASSERT_NE", b, nullptr)) { return; }
    if (!TestNotEqual("ASSERT_NE", c, nullptr)) { return; }
    TestEqual("EXPECT_EQ", *a, "1");
    TestEqual("EXPECT_EQ", *b, "2");
    TestEqual("EXPECT_EQ", *c, "3");
}

// =============================================================
// Comments and blank lines
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, SkipComments)
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
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }
    TestNotEqual("EXPECT_NE", sec->GetValueString("Key"), nullptr);
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, SkipBlankLines)
{
    FConfigFile config;
    config.ReadFromString(
        "\n\n[Section]\n\nKey=Value\n\n"
    );

    const FConfigSection* sec = config.FindSection("Section");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }
    TestNotEqual("EXPECT_NE", sec->GetValueString("Key"), nullptr);
}

// =============================================================
// Array operators
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ArrayAddUniqueOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n+Items=Apple\n+Items=Banana\n");

    const FConfigSection* sec = config.FindSection("S");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    auto values = sec->GetValues("Items");
    TestEqual("EXPECT_EQ", values.size(), 2u);
    TestEqual("EXPECT_EQ", values[0]->Value, "Apple");
    TestEqual("EXPECT_EQ", values[1]->Value, "Banana");
    TestEqual("EXPECT_EQ", values[0]->ValueType, EConfigValueType::ArrayAddUnique);
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ArrayAddOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n.Items=Apple\n.Items=Apple\n");

    const FConfigSection* sec = config.FindSection("S");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    auto values = sec->GetValues("Items");
    TestEqual("EXPECT_EQ", values.size(), 2u);
    TestEqual("EXPECT_EQ", values[0]->ValueType, EConfigValueType::ArrayAdd);
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, RemoveOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n-Items=Apple\n");

    const FConfigSection* sec = config.FindSection("S");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    auto values = sec->GetValues("Items");
    TestEqual("EXPECT_EQ", values.size(), 1u);
    TestEqual("EXPECT_EQ", values[0]->ValueType, EConfigValueType::Remove);
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ClearOperator)
{
    FConfigFile config;
    config.ReadFromString("[S]\n!Items=ClearArray\n");

    const FConfigSection* sec = config.FindSection("S");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    auto values = sec->GetValues("Items");
    TestEqual("EXPECT_EQ", values.size(), 1u);
    TestEqual("EXPECT_EQ", values[0]->ValueType, EConfigValueType::Clear);
}

// =============================================================
// Whitespace trimming
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, TrimWhitespace)
{
    FConfigFile config;
    config.ReadFromString("  [ Section ]  \n  Key  =  Value  \n");

    TestTrue("EXPECT_TRUE", config.ContainsSection("Section"));
    const FConfigSection* sec = config.FindSection("Section");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    const std::string* val = sec->GetValueString("Key");
    if (!TestNotEqual("ASSERT_NE", val, nullptr)) { return; }
    TestEqual("EXPECT_EQ", *val, "Value");
}

// =============================================================
// Malformed input recovery
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, MalformedSectionHeader)
{
    FConfigFile config;
    // Missing closing bracket -- should log warning but not crash.
    config.ReadFromString("[Broken\nKey=Value\n[Good]\nA=B\n");

    // "Broken" section may or may not be created depending on impl.
    // "Good" section should exist.
    TestTrue("EXPECT_TRUE", config.ContainsSection("Good"));
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, LineWithoutEquals)
{
    FConfigFile config;
    // No '=' sign -- should log warning and skip.
    config.ReadFromString("[S]\nNoEqualsHere\nKey=Value\n");

    const FConfigSection* sec = config.FindSection("S");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }

    const std::string* val = sec->GetValueString("Key");
    if (!TestNotEqual("ASSERT_NE", val, nullptr)) { return; }
    TestEqual("EXPECT_EQ", *val, "Value");
}

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, KeyValueBeforeAnySection)
{
    FConfigFile config;
    // Key before any section header -- should log warning and skip.
    config.ReadFromString("Orphan=Value\n[S]\nKey=Value\n");

    TestTrue("EXPECT_TRUE", config.ContainsSection("S"));
}

// =============================================================
// Round-trip: ReadFromString -> WriteToString -> ReadFromString
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, RoundTrip)
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

    TestEqual("EXPECT_EQ", config1, config2);
}

// =============================================================
// File I/O (non-existent file)
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, ReadNonExistentFile)
{
    FConfigFile config;
    TestFalse("EXPECT_FALSE", config.Read("__nonexistent_path_12345__.ini"));
}

// =============================================================
// FindOrAddSection
// =============================================================

ENIGMA_IMPLEMENT_CORE_CONFIG_AUTOMATION_TEST(ConfigFileTest, FindOrAddSection)
{
    FConfigFile config;
    TestTrue("EXPECT_TRUE", config.IsEmpty());

    FConfigSection* sec = config.FindOrAddSection("NewSection");
    if (!TestNotEqual("ASSERT_NE", sec, nullptr)) { return; }
    TestTrue("EXPECT_TRUE", config.ContainsSection("NewSection"));
    TestFalse("EXPECT_FALSE", config.IsEmpty());
}
