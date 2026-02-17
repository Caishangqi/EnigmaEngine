// Copyright EnigmaEngine. All Rights Reserved.

/// @file FColorTest.cpp
/// @brief Unit tests for FColor.

#include <gtest/gtest.h>
#include "Math/Color.h"
#include "Math/LinearColor.h"

using Enigma::FColor;
using Enigma::FLinearColor;

// =================================================================
// Constructors
// =================================================================

TEST(FColorTest, DefaultConstructor)
{
	constexpr FColor C;
	EXPECT_EQ(C.R, 0);
	EXPECT_EQ(C.G, 0);
	EXPECT_EQ(C.B, 0);
	EXPECT_EQ(C.A, 255);
}

TEST(FColorTest, ComponentConstructor)
{
	constexpr FColor C(128, 64, 32, 200);
	EXPECT_EQ(C.R, 128);
	EXPECT_EQ(C.G, 64);
	EXPECT_EQ(C.B, 32);
	EXPECT_EQ(C.A, 200);
}

TEST(FColorTest, ComponentConstructorDefaultAlpha)
{
	constexpr FColor C(100, 150, 200);
	EXPECT_EQ(C.A, 255);
}

TEST(FColorTest, FromLinearColorWhite)
{
	const FColor C(FLinearColor::White);
	EXPECT_EQ(C.R, 255);
	EXPECT_EQ(C.G, 255);
	EXPECT_EQ(C.B, 255);
	EXPECT_EQ(C.A, 255);
}

TEST(FColorTest, FromLinearColorBlack)
{
	const FColor C(FLinearColor::Black);
	EXPECT_EQ(C.R, 0);
	EXPECT_EQ(C.G, 0);
	EXPECT_EQ(C.B, 0);
	EXPECT_EQ(C.A, 255);
}

// =================================================================
// Comparison
// =================================================================

TEST(FColorTest, EqualityOperator)
{
	constexpr FColor A(255, 128, 64, 255);
	constexpr FColor B(255, 128, 64, 255);
	constexpr FColor C(255, 128, 65, 255);
	EXPECT_TRUE(A == B);
	EXPECT_FALSE(A == C);
	EXPECT_TRUE(A != C);
}

// =================================================================
// ToLinearColor
// =================================================================

TEST(FColorTest, ToLinearColorWhite)
{
	const FLinearColor L = FColor::White.ToLinearColor();
	EXPECT_NEAR(L.R, 1.0f, 1e-4f);
	EXPECT_NEAR(L.G, 1.0f, 1e-4f);
	EXPECT_NEAR(L.B, 1.0f, 1e-4f);
	EXPECT_NEAR(L.A, 1.0f, 1e-4f);
}

TEST(FColorTest, ToLinearColorBlack)
{
	const FLinearColor L = FColor::Black.ToLinearColor();
	EXPECT_NEAR(L.R, 0.0f, 1e-4f);
	EXPECT_NEAR(L.G, 0.0f, 1e-4f);
	EXPECT_NEAR(L.B, 0.0f, 1e-4f);
}

TEST(FColorTest, ToLinearColorMidGray)
{
	// sRGB 128 ~= linear 0.2158
	const FLinearColor L = FColor(128, 128, 128, 255).ToLinearColor();
	EXPECT_NEAR(L.R, 0.2158f, 0.01f);
}

// =================================================================
// sRGB roundtrip (FColor -> FLinearColor -> FColor)
// =================================================================

TEST(FColorTest, SRGBRoundtripPureColors)
{
	const FColor Colors[] = {
		FColor::White, FColor::Black, FColor::Red,
		FColor::Green, FColor::Blue, FColor::Yellow
	};
	for (const FColor& Original : Colors)
	{
		const FLinearColor Linear = Original.ToLinearColor();
		const FColor Back = Linear.ToFColor(true);
		EXPECT_NEAR(static_cast<int>(Back.R), static_cast<int>(Original.R), 1);
		EXPECT_NEAR(static_cast<int>(Back.G), static_cast<int>(Original.G), 1);
		EXPECT_NEAR(static_cast<int>(Back.B), static_cast<int>(Original.B), 1);
		EXPECT_NEAR(static_cast<int>(Back.A), static_cast<int>(Original.A), 1);
	}
}

TEST(FColorTest, SRGBRoundtripArbitraryValues)
{
	// Test a range of arbitrary values. Allow +/-1 for quantization.
	const FColor Original(73, 142, 211, 180);
	const FLinearColor Linear = Original.ToLinearColor();
	const FColor Back = Linear.ToFColor(true);
	EXPECT_NEAR(static_cast<int>(Back.R), 73, 1);
	EXPECT_NEAR(static_cast<int>(Back.G), 142, 1);
	EXPECT_NEAR(static_cast<int>(Back.B), 211, 1);
	EXPECT_NEAR(static_cast<int>(Back.A), 180, 1);
}

// =================================================================
// ToHex
// =================================================================

TEST(FColorTest, ToHexWhite)
{
	EXPECT_EQ(FColor::White.ToHex(), "#FFFFFFFF");
}

TEST(FColorTest, ToHexBlack)
{
	EXPECT_EQ(FColor::Black.ToHex(), "#000000FF");
}

TEST(FColorTest, ToHexRed)
{
	EXPECT_EQ(FColor::Red.ToHex(), "#FF0000FF");
}

TEST(FColorTest, ToHexTransparent)
{
	EXPECT_EQ(FColor::Transparent.ToHex(), "#00000000");
}

TEST(FColorTest, ToHexArbitrary)
{
	const FColor C(0x1A, 0x2B, 0x3C, 0x4D);
	EXPECT_EQ(C.ToHex(), "#1A2B3C4D");
}

TEST(FColorTest, ToHexLeadingZeros)
{
	const FColor C(1, 2, 3, 4);
	EXPECT_EQ(C.ToHex(), "#01020304");
}

// =================================================================
// Constants
// =================================================================

TEST(FColorTest, Constants)
{
	EXPECT_EQ(FColor::White, FColor(255, 255, 255, 255));
	EXPECT_EQ(FColor::Black, FColor(0, 0, 0, 255));
	EXPECT_EQ(FColor::Red, FColor(255, 0, 0, 255));
	EXPECT_EQ(FColor::Green, FColor(0, 255, 0, 255));
	EXPECT_EQ(FColor::Blue, FColor(0, 0, 255, 255));
	EXPECT_EQ(FColor::Yellow, FColor(255, 255, 0, 255));
	EXPECT_EQ(FColor::Transparent, FColor(0, 0, 0, 0));
}
