// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiCellTest.cpp
/// @brief Unit tests for FAsciiCell default/custom construction and transparency.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.AsciiRenderer." #SuiteName "." #TestName, \
        AsciiRenderer, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#define ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.AsciiRenderer." #FixtureName "." #TestName, \
        AsciiRenderer, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

using namespace Enigma;

// ---------------------------------------------------------------
// Default construction
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, DefaultConstruction_Character)
{
	FAsciiCell cell;
	TestEqual("EXPECT_EQ", cell.Character, ' ');
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, DefaultConstruction_Foreground)
{
	FAsciiCell cell;
	TestEqual("EXPECT_EQ", cell.Foreground, FColor::White);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, DefaultConstruction_Background)
{
	FAsciiCell cell;
	TestEqual("EXPECT_EQ", cell.Background, FColor::Black);
}

// ---------------------------------------------------------------
// Custom construction
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, CustomConstruction)
{
	FAsciiCell cell{'@', FColor::Red, FColor::Blue};
	TestEqual("EXPECT_EQ", cell.Character, '@');
	TestEqual("EXPECT_EQ", cell.Foreground, FColor::Red);
	TestEqual("EXPECT_EQ", cell.Background, FColor::Blue);
}

// ---------------------------------------------------------------
// Transparency
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, IsTransparent_NullChar)
{
	FAsciiCell cell{'\0', FColor::White, FColor::Black};
	TestTrue("EXPECT_TRUE", cell.IsTransparent());
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, IsTransparent_Space_NotTransparent)
{
	FAsciiCell cell{' ', FColor::White, FColor::Black};
	TestFalse("EXPECT_FALSE", cell.IsTransparent());
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, IsTransparent_VisibleChar_NotTransparent)
{
	FAsciiCell cell{'A', FColor::White, FColor::Black};
	TestFalse("EXPECT_FALSE", cell.IsTransparent());
}

// ---------------------------------------------------------------
// Equality
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, Equality_Same)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'X', FColor::Red, FColor::Green};
	TestEqual("EXPECT_EQ", a, b);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, Inequality_DifferentChar)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'Y', FColor::Red, FColor::Green};
	TestNotEqual("EXPECT_NE", a, b);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, Inequality_DifferentFg)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'X', FColor::Blue, FColor::Green};
	TestNotEqual("EXPECT_NE", a, b);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiCellTest, Inequality_DifferentBg)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'X', FColor::Red, FColor::Blue};
	TestNotEqual("EXPECT_NE", a, b);
}
