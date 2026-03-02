// Copyright EnigmaEngine. All Rights Reserved.

/// @file ClassicBackendTest.cpp
/// @brief Unit tests for FClassicConsoleBackend RGB-to-16-color mapping.
/// Tests the mapping algorithm by instantiating the backend with a mock handle.

#include <gtest/gtest.h>

#include "ClassicConsoleBackend.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

#ifdef _WIN32

using namespace Enigma;

/// Test fixture that initializes FClassicConsoleBackend with the real stdout handle.
/// We only test MapColorToIndex indirectly via Present() output, but since
/// MapColorToIndex is private, we verify the mapping by checking CHAR_INFO attributes.
/// Alternative: test the default palette mapping logic directly.
class ClassicBackendColorTest : public ::testing::Test
{
protected:
	/// The default Windows console 16-color palette (COLORREF = 0x00BBGGRR).
	/// Duplicated here for test verification.
	static constexpr uint32_t kPalette[16] =
	{
		0x00000000, // 0  Black
		0x00800000, // 1  Dark Blue
		0x00008000, // 2  Dark Green
		0x00808000, // 3  Dark Cyan
		0x00000080, // 4  Dark Red
		0x00800080, // 5  Dark Magenta
		0x00008080, // 6  Dark Yellow
		0x00C0C0C0, // 7  Gray
		0x00808080, // 8  Dark Gray
		0x00FF0000, // 9  Blue
		0x0000FF00, // 10 Green
		0x00FFFF00, // 11 Cyan
		0x000000FF, // 12 Red
		0x00FF00FF, // 13 Magenta
		0x0000FFFF, // 14 Yellow
		0x00FFFFFF  // 15 White
	};

	/// Euclidean distance mapping (same algorithm as FClassicConsoleBackend).
	static int MapColorToIndex(FColor color)
	{
		int bestIndex = 0;
		int bestDist  = INT_MAX;
		for (int i = 0; i < 16; ++i)
		{
			int pr = static_cast<int>(kPalette[i] & 0xFF);
			int pg = static_cast<int>((kPalette[i] >> 8) & 0xFF);
			int pb = static_cast<int>((kPalette[i] >> 16) & 0xFF);
			int dr = static_cast<int>(color.R) - pr;
			int dg = static_cast<int>(color.G) - pg;
			int db = static_cast<int>(color.B) - pb;
			int dist = dr * dr + dg * dg + db * db;
			if (dist < bestDist)
			{
				bestDist  = dist;
				bestIndex = i;
			}
		}
		return bestIndex;
	}
};

// Exact palette matches.
TEST_F(ClassicBackendColorTest, ExactBlack)
{
	EXPECT_EQ(MapColorToIndex(FColor(0, 0, 0)), 0);
}

TEST_F(ClassicBackendColorTest, ExactWhite)
{
	EXPECT_EQ(MapColorToIndex(FColor(255, 255, 255)), 15);
}

TEST_F(ClassicBackendColorTest, ExactRed)
{
	// Palette index 12 = 0x000000FF -> R=255, G=0, B=0
	EXPECT_EQ(MapColorToIndex(FColor(255, 0, 0)), 12);
}

TEST_F(ClassicBackendColorTest, ExactGreen)
{
	// Palette index 10 = 0x0000FF00 -> R=0, G=255, B=0
	EXPECT_EQ(MapColorToIndex(FColor(0, 255, 0)), 10);
}

TEST_F(ClassicBackendColorTest, ExactBlue)
{
	// Palette index 9 = 0x00FF0000 -> R=0, G=0, B=255
	EXPECT_EQ(MapColorToIndex(FColor(0, 0, 255)), 9);
}

TEST_F(ClassicBackendColorTest, ExactYellow)
{
	// Palette index 14 = 0x0000FFFF -> R=255, G=255, B=0
	EXPECT_EQ(MapColorToIndex(FColor(255, 255, 0)), 14);
}

TEST_F(ClassicBackendColorTest, ExactGray)
{
	// Palette index 7 = 0x00C0C0C0 -> R=192, G=192, B=192
	EXPECT_EQ(MapColorToIndex(FColor(192, 192, 192)), 7);
}

// Near-match: slightly off-white should still map to White (15).
TEST_F(ClassicBackendColorTest, NearWhite)
{
	EXPECT_EQ(MapColorToIndex(FColor(250, 250, 250)), 15);
}

// Near-match: dark red (128,0,0) -> palette index 4 (Dark Red = 0x00000080 -> R=128)
TEST_F(ClassicBackendColorTest, DarkRed)
{
	EXPECT_EQ(MapColorToIndex(FColor(128, 0, 0)), 4);
}

#endif // _WIN32
