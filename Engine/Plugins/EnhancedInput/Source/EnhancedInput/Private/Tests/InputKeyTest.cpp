// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputKeyTest.cpp
/// @brief Unit tests for FKey and EKeys.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.EnhancedInput." #SuiteName "." #TestName, \
        EnhancedInput, \
        ::Enigma::EAutomationTestType::Unit, \
        ::Enigma::EAutomationTestFlags::None)

#define ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.EnhancedInput." #FixtureName "." #TestName, \
        EnhancedInput, \
        ::Enigma::EAutomationTestType::Unit, \
        ::Enigma::EAutomationTestFlags::None)
#include "InputKeys.h"

using namespace Enigma;

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, DefaultConstructedIsInvalid)
{
	FKey key;
	TestFalse("EXPECT_FALSE", key.IsValid());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, NamedKeyIsValid)
{
	FKey key("TestKey");
	TestTrue("EXPECT_TRUE", key.IsValid());
	TestEqual("EXPECT_EQ", key.GetKeyName(), "TestKey");
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, EqualityComparison)
{
	TestEqual("EXPECT_EQ", FKey("A"), EKeys::A);
	TestNotEqual("EXPECT_NE", EKeys::A, EKeys::B);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, HashWorksInContainer)
{
	std::unordered_map<FKey, int, FKey::Hash> map;
	map[EKeys::A] = 1;
	map[EKeys::B] = 2;
	TestEqual("EXPECT_EQ", map[EKeys::A], 1);
	TestEqual("EXPECT_EQ", map[EKeys::B], 2);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, IsAxisKey)
{
	TestTrue("EXPECT_TRUE", EKeys::MouseX.IsAxisKey());
	TestTrue("EXPECT_TRUE", EKeys::MouseY.IsAxisKey());
	TestTrue("EXPECT_TRUE", EKeys::MouseWheelAxis.IsAxisKey());
	TestTrue("EXPECT_TRUE", EKeys::Gamepad_LeftX.IsAxisKey());
	TestFalse("EXPECT_FALSE", EKeys::A.IsAxisKey());
	TestFalse("EXPECT_FALSE", EKeys::LeftMouseButton.IsAxisKey());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, IsMouseKey)
{
	TestTrue("EXPECT_TRUE", EKeys::LeftMouseButton.IsMouseKey());
	TestTrue("EXPECT_TRUE", EKeys::RightMouseButton.IsMouseKey());
	TestTrue("EXPECT_TRUE", EKeys::MouseX.IsMouseKey());
	TestFalse("EXPECT_FALSE", EKeys::A.IsMouseKey());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, IsModifierKey)
{
	TestTrue("EXPECT_TRUE", EKeys::LeftShift.IsModifierKey());
	TestTrue("EXPECT_TRUE", EKeys::LeftControl.IsModifierKey());
	TestTrue("EXPECT_TRUE", EKeys::LeftAlt.IsModifierKey());
	TestFalse("EXPECT_FALSE", EKeys::A.IsModifierKey());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, TranslateKeyCodeLetters)
{
	// VK_A = 0x41
	FKey a = TranslateKeyCode(0x41);
	TestEqual("EXPECT_EQ", a, EKeys::A);

	// VK_Z = 0x5A
	FKey z = TranslateKeyCode(0x5A);
	TestEqual("EXPECT_EQ", z, EKeys::Z);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, TranslateKeyCodeSpecialKeys)
{
	// VK_ESCAPE = 0x1B
	TestEqual("EXPECT_EQ", TranslateKeyCode(0x1B), EKeys::Escape);
	// VK_TAB = 0x09
	TestEqual("EXPECT_EQ", TranslateKeyCode(0x09), EKeys::Tab);
	// VK_SPACE = 0x20
	TestEqual("EXPECT_EQ", TranslateKeyCode(0x20), EKeys::SpaceBar);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputKeyTest, TranslateKeyCodeUnknownReturnsNone)
{
	FKey unknown = TranslateKeyCode(0xFF);
	TestEqual("EXPECT_EQ", unknown, EKeys::None);
}
