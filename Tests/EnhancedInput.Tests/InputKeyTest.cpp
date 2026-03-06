// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputKeyTest.cpp
/// @brief Unit tests for FKey and EKeys.

#include <gtest/gtest.h>
#include "InputKeys.h"

using namespace Enigma;

TEST(InputKeyTest, DefaultConstructedIsInvalid)
{
	FKey key;
	EXPECT_FALSE(key.IsValid());
}

TEST(InputKeyTest, NamedKeyIsValid)
{
	FKey key("TestKey");
	EXPECT_TRUE(key.IsValid());
	EXPECT_EQ(key.GetKeyName(), "TestKey");
}

TEST(InputKeyTest, EqualityComparison)
{
	EXPECT_EQ(FKey("A"), EKeys::A);
	EXPECT_NE(EKeys::A, EKeys::B);
}

TEST(InputKeyTest, HashWorksInContainer)
{
	std::unordered_map<FKey, int, FKey::Hash> map;
	map[EKeys::A] = 1;
	map[EKeys::B] = 2;
	EXPECT_EQ(map[EKeys::A], 1);
	EXPECT_EQ(map[EKeys::B], 2);
}

TEST(InputKeyTest, IsAxisKey)
{
	EXPECT_TRUE(EKeys::MouseX.IsAxisKey());
	EXPECT_TRUE(EKeys::MouseY.IsAxisKey());
	EXPECT_TRUE(EKeys::MouseWheelAxis.IsAxisKey());
	EXPECT_TRUE(EKeys::Gamepad_LeftX.IsAxisKey());
	EXPECT_FALSE(EKeys::A.IsAxisKey());
	EXPECT_FALSE(EKeys::LeftMouseButton.IsAxisKey());
}

TEST(InputKeyTest, IsMouseKey)
{
	EXPECT_TRUE(EKeys::LeftMouseButton.IsMouseKey());
	EXPECT_TRUE(EKeys::RightMouseButton.IsMouseKey());
	EXPECT_TRUE(EKeys::MouseX.IsMouseKey());
	EXPECT_FALSE(EKeys::A.IsMouseKey());
}

TEST(InputKeyTest, IsModifierKey)
{
	EXPECT_TRUE(EKeys::LeftShift.IsModifierKey());
	EXPECT_TRUE(EKeys::LeftControl.IsModifierKey());
	EXPECT_TRUE(EKeys::LeftAlt.IsModifierKey());
	EXPECT_FALSE(EKeys::A.IsModifierKey());
}

TEST(InputKeyTest, TranslateKeyCodeLetters)
{
	// VK_A = 0x41
	FKey a = TranslateKeyCode(0x41);
	EXPECT_EQ(a, EKeys::A);

	// VK_Z = 0x5A
	FKey z = TranslateKeyCode(0x5A);
	EXPECT_EQ(z, EKeys::Z);
}

TEST(InputKeyTest, TranslateKeyCodeSpecialKeys)
{
	// VK_ESCAPE = 0x1B
	EXPECT_EQ(TranslateKeyCode(0x1B), EKeys::Escape);
	// VK_TAB = 0x09
	EXPECT_EQ(TranslateKeyCode(0x09), EKeys::Tab);
	// VK_SPACE = 0x20
	EXPECT_EQ(TranslateKeyCode(0x20), EKeys::SpaceBar);
}

TEST(InputKeyTest, TranslateKeyCodeUnknownReturnsNone)
{
	FKey unknown = TranslateKeyCode(0xFF);
	EXPECT_EQ(unknown, EKeys::None);
}
