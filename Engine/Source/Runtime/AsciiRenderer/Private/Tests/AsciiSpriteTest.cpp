// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiSpriteTest.cpp
/// @brief Unit tests for FAsciiSprite creation, At() access, and bounds.

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

#include "RenderCore/AsciiSprite.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

using namespace Enigma;

// ---------------------------------------------------------------
// Construction
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteTest, DefaultConstruction_ZeroDimensions)
{
	FAsciiSprite sprite;
	TestEqual("EXPECT_EQ", sprite.Width, 0);
	TestEqual("EXPECT_EQ", sprite.Height, 0);
	TestTrue("EXPECT_TRUE", sprite.Cells.empty());
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteTest, SizedConstruction_Dimensions)
{
	FAsciiSprite sprite(4, 3);
	TestEqual("EXPECT_EQ", sprite.Width, 4);
	TestEqual("EXPECT_EQ", sprite.Height, 3);
	TestEqual("EXPECT_EQ", sprite.Cells.size(), 12u);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteTest, SizedConstruction_DefaultCells)
{
	FAsciiSprite sprite(2, 2);
	FAsciiCell defaultCell;
	for (const auto& cell : sprite.Cells)
	{
		TestEqual("EXPECT_EQ", cell, defaultCell);
	}
}

// ---------------------------------------------------------------
// At() access
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteTest, At_WriteAndRead)
{
	FAsciiSprite sprite(3, 2);
	FAsciiCell custom{'@', FColor::Red, FColor::Blue};
	sprite.At(1, 0) = custom;
	TestEqual("EXPECT_EQ", sprite.At(1, 0), custom);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteTest, At_ConstAccess)
{
	FAsciiSprite sprite(3, 2);
	sprite.At(2, 1) = FAsciiCell{'#', FColor::Green, FColor::Black};

	const FAsciiSprite& constRef = sprite;
	TestEqual("EXPECT_EQ", constRef.At(2, 1).Character, '#');
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteTest, At_RowMajorLayout)
{
	FAsciiSprite sprite(3, 2);
	// Row-major: At(x, y) = Cells[y * Width + x]
	sprite.At(0, 0) = FAsciiCell{'A', FColor::White, FColor::Black};
	sprite.At(1, 0) = FAsciiCell{'B', FColor::White, FColor::Black};
	sprite.At(2, 0) = FAsciiCell{'C', FColor::White, FColor::Black};
	sprite.At(0, 1) = FAsciiCell{'D', FColor::White, FColor::Black};

	TestEqual("EXPECT_EQ", sprite.Cells[0].Character, 'A');
	TestEqual("EXPECT_EQ", sprite.Cells[1].Character, 'B');
	TestEqual("EXPECT_EQ", sprite.Cells[2].Character, 'C');
	TestEqual("EXPECT_EQ", sprite.Cells[3].Character, 'D');
}
