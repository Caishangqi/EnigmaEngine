// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiSpriteTest.cpp
/// @brief Unit tests for FAsciiSprite creation, At() access, and bounds.

#include <gtest/gtest.h>

#include "RenderCore/AsciiSprite.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

using namespace Enigma;

// ---------------------------------------------------------------
// Construction
// ---------------------------------------------------------------

TEST(AsciiSpriteTest, DefaultConstruction_ZeroDimensions)
{
	FAsciiSprite sprite;
	EXPECT_EQ(sprite.Width, 0);
	EXPECT_EQ(sprite.Height, 0);
	EXPECT_TRUE(sprite.Cells.empty());
}

TEST(AsciiSpriteTest, SizedConstruction_Dimensions)
{
	FAsciiSprite sprite(4, 3);
	EXPECT_EQ(sprite.Width, 4);
	EXPECT_EQ(sprite.Height, 3);
	EXPECT_EQ(sprite.Cells.size(), 12u);
}

TEST(AsciiSpriteTest, SizedConstruction_DefaultCells)
{
	FAsciiSprite sprite(2, 2);
	FAsciiCell defaultCell;
	for (const auto& cell : sprite.Cells)
	{
		EXPECT_EQ(cell, defaultCell);
	}
}

// ---------------------------------------------------------------
// At() access
// ---------------------------------------------------------------

TEST(AsciiSpriteTest, At_WriteAndRead)
{
	FAsciiSprite sprite(3, 2);
	FAsciiCell custom{'@', FColor::Red, FColor::Blue};
	sprite.At(1, 0) = custom;
	EXPECT_EQ(sprite.At(1, 0), custom);
}

TEST(AsciiSpriteTest, At_ConstAccess)
{
	FAsciiSprite sprite(3, 2);
	sprite.At(2, 1) = FAsciiCell{'#', FColor::Green, FColor::Black};

	const FAsciiSprite& constRef = sprite;
	EXPECT_EQ(constRef.At(2, 1).Character, '#');
}

TEST(AsciiSpriteTest, At_RowMajorLayout)
{
	FAsciiSprite sprite(3, 2);
	// Row-major: At(x, y) = Cells[y * Width + x]
	sprite.At(0, 0) = FAsciiCell{'A', FColor::White, FColor::Black};
	sprite.At(1, 0) = FAsciiCell{'B', FColor::White, FColor::Black};
	sprite.At(2, 0) = FAsciiCell{'C', FColor::White, FColor::Black};
	sprite.At(0, 1) = FAsciiCell{'D', FColor::White, FColor::Black};

	EXPECT_EQ(sprite.Cells[0].Character, 'A');
	EXPECT_EQ(sprite.Cells[1].Character, 'B');
	EXPECT_EQ(sprite.Cells[2].Character, 'C');
	EXPECT_EQ(sprite.Cells[3].Character, 'D');
}
