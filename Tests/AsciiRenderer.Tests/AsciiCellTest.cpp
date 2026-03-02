// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiCellTest.cpp
/// @brief Unit tests for FAsciiCell default/custom construction and transparency.

#include <gtest/gtest.h>

#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

using namespace Enigma;

// ---------------------------------------------------------------
// Default construction
// ---------------------------------------------------------------

TEST(AsciiCellTest, DefaultConstruction_Character)
{
	FAsciiCell cell;
	EXPECT_EQ(cell.Character, ' ');
}

TEST(AsciiCellTest, DefaultConstruction_Foreground)
{
	FAsciiCell cell;
	EXPECT_EQ(cell.Foreground, FColor::White);
}

TEST(AsciiCellTest, DefaultConstruction_Background)
{
	FAsciiCell cell;
	EXPECT_EQ(cell.Background, FColor::Black);
}

// ---------------------------------------------------------------
// Custom construction
// ---------------------------------------------------------------

TEST(AsciiCellTest, CustomConstruction)
{
	FAsciiCell cell{'@', FColor::Red, FColor::Blue};
	EXPECT_EQ(cell.Character, '@');
	EXPECT_EQ(cell.Foreground, FColor::Red);
	EXPECT_EQ(cell.Background, FColor::Blue);
}

// ---------------------------------------------------------------
// Transparency
// ---------------------------------------------------------------

TEST(AsciiCellTest, IsTransparent_NullChar)
{
	FAsciiCell cell{'\0', FColor::White, FColor::Black};
	EXPECT_TRUE(cell.IsTransparent());
}

TEST(AsciiCellTest, IsTransparent_Space_NotTransparent)
{
	FAsciiCell cell{' ', FColor::White, FColor::Black};
	EXPECT_FALSE(cell.IsTransparent());
}

TEST(AsciiCellTest, IsTransparent_VisibleChar_NotTransparent)
{
	FAsciiCell cell{'A', FColor::White, FColor::Black};
	EXPECT_FALSE(cell.IsTransparent());
}

// ---------------------------------------------------------------
// Equality
// ---------------------------------------------------------------

TEST(AsciiCellTest, Equality_Same)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'X', FColor::Red, FColor::Green};
	EXPECT_EQ(a, b);
}

TEST(AsciiCellTest, Inequality_DifferentChar)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'Y', FColor::Red, FColor::Green};
	EXPECT_NE(a, b);
}

TEST(AsciiCellTest, Inequality_DifferentFg)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'X', FColor::Blue, FColor::Green};
	EXPECT_NE(a, b);
}

TEST(AsciiCellTest, Inequality_DifferentBg)
{
	FAsciiCell a{'X', FColor::Red, FColor::Green};
	FAsciiCell b{'X', FColor::Red, FColor::Blue};
	EXPECT_NE(a, b);
}
