// Copyright EnigmaEngine. All Rights Reserved.

/// @file FColorTest.cpp
/// @brief Unit tests for FColor.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(SuiteName, TestName)                                               \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST(                                      \
		F##SuiteName##_##TestName##AutomationTest,                                        \
		"System.Core.Math." #SuiteName "." #TestName,                                      \
		Core,                                                                    \
		::Enigma::EAutomationTestType::Unit,                                                       \
		::Enigma::EAutomationTestFlags::None)

#define ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST_F(FixtureName, TestName)                                      \
	ENIGMA_IMPLEMENT_AUTOMATION_TEST_F(                                    \
		FixtureName,                                                                     \
		F##FixtureName##_##TestName##AutomationTest,                                     \
		"System.Core.Math." #FixtureName "." #TestName,                                    \
		Core,                                                                    \
		::Enigma::EAutomationTestType::Unit,                                                       \
		::Enigma::EAutomationTestFlags::None)
#include "Math/Color.h"
#include "Math/LinearColor.h"

using Enigma::FColor;
using Enigma::FLinearColor;

// =================================================================
// Constructors
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, DefaultConstructor)
{
	constexpr FColor C;
	TestEqual("EXPECT_EQ", C.R, 0);
	TestEqual("EXPECT_EQ", C.G, 0);
	TestEqual("EXPECT_EQ", C.B, 0);
	TestEqual("EXPECT_EQ", C.A, 255);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ComponentConstructor)
{
	constexpr FColor C(128, 64, 32, 200);
	TestEqual("EXPECT_EQ", C.R, 128);
	TestEqual("EXPECT_EQ", C.G, 64);
	TestEqual("EXPECT_EQ", C.B, 32);
	TestEqual("EXPECT_EQ", C.A, 200);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ComponentConstructorDefaultAlpha)
{
	constexpr FColor C(100, 150, 200);
	TestEqual("EXPECT_EQ", C.A, 255);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, FromLinearColorWhite)
{
	const FColor C(FLinearColor::White);
	TestEqual("EXPECT_EQ", C.R, 255);
	TestEqual("EXPECT_EQ", C.G, 255);
	TestEqual("EXPECT_EQ", C.B, 255);
	TestEqual("EXPECT_EQ", C.A, 255);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, FromLinearColorBlack)
{
	const FColor C(FLinearColor::Black);
	TestEqual("EXPECT_EQ", C.R, 0);
	TestEqual("EXPECT_EQ", C.G, 0);
	TestEqual("EXPECT_EQ", C.B, 0);
	TestEqual("EXPECT_EQ", C.A, 255);
}

// =================================================================
// Comparison
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, EqualityOperator)
{
	constexpr FColor A(255, 128, 64, 255);
	constexpr FColor B(255, 128, 64, 255);
	constexpr FColor C(255, 128, 65, 255);
	TestTrue("EXPECT_TRUE", A == B);
	TestFalse("EXPECT_FALSE", A == C);
	TestTrue("EXPECT_TRUE", A != C);
}

// =================================================================
// ToLinearColor
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToLinearColorWhite)
{
	const FLinearColor L = FColor::White.ToLinearColor();
	TestNear("EXPECT_NEAR", L.R, 1.0f, 1e-4f);
	TestNear("EXPECT_NEAR", L.G, 1.0f, 1e-4f);
	TestNear("EXPECT_NEAR", L.B, 1.0f, 1e-4f);
	TestNear("EXPECT_NEAR", L.A, 1.0f, 1e-4f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToLinearColorBlack)
{
	const FLinearColor L = FColor::Black.ToLinearColor();
	TestNear("EXPECT_NEAR", L.R, 0.0f, 1e-4f);
	TestNear("EXPECT_NEAR", L.G, 0.0f, 1e-4f);
	TestNear("EXPECT_NEAR", L.B, 0.0f, 1e-4f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToLinearColorMidGray)
{
	// sRGB 128 ~= linear 0.2158
	const FLinearColor L = FColor(128, 128, 128, 255).ToLinearColor();
	TestNear("EXPECT_NEAR", L.R, 0.2158f, 0.01f);
}

// =================================================================
// sRGB roundtrip (FColor -> FLinearColor -> FColor)
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, SRGBRoundtripPureColors)
{
	const FColor Colors[] = {
		FColor::White, FColor::Black, FColor::Red,
		FColor::Green, FColor::Blue, FColor::Yellow
	};
	for (const FColor& Original : Colors)
	{
		const FLinearColor Linear = Original.ToLinearColor();
		const FColor Back = Linear.ToFColor(true);
		TestNear("EXPECT_NEAR", static_cast<int>(Back.R), static_cast<int>(Original.R), 1);
		TestNear("EXPECT_NEAR", static_cast<int>(Back.G), static_cast<int>(Original.G), 1);
		TestNear("EXPECT_NEAR", static_cast<int>(Back.B), static_cast<int>(Original.B), 1);
		TestNear("EXPECT_NEAR", static_cast<int>(Back.A), static_cast<int>(Original.A), 1);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, SRGBRoundtripArbitraryValues)
{
	// Test a range of arbitrary values. Allow +/-1 for quantization.
	const FColor Original(73, 142, 211, 180);
	const FLinearColor Linear = Original.ToLinearColor();
	const FColor Back = Linear.ToFColor(true);
	TestNear("EXPECT_NEAR", static_cast<int>(Back.R), 73, 1);
	TestNear("EXPECT_NEAR", static_cast<int>(Back.G), 142, 1);
	TestNear("EXPECT_NEAR", static_cast<int>(Back.B), 211, 1);
	TestNear("EXPECT_NEAR", static_cast<int>(Back.A), 180, 1);
}

// =================================================================
// ToHex
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToHexWhite)
{
	TestEqual("EXPECT_EQ", FColor::White.ToHex(), "#FFFFFFFF");
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToHexBlack)
{
	TestEqual("EXPECT_EQ", FColor::Black.ToHex(), "#000000FF");
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToHexRed)
{
	TestEqual("EXPECT_EQ", FColor::Red.ToHex(), "#FF0000FF");
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToHexTransparent)
{
	TestEqual("EXPECT_EQ", FColor::Transparent.ToHex(), "#00000000");
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToHexArbitrary)
{
	const FColor C(0x1A, 0x2B, 0x3C, 0x4D);
	TestEqual("EXPECT_EQ", C.ToHex(), "#1A2B3C4D");
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, ToHexLeadingZeros)
{
	const FColor C(1, 2, 3, 4);
	TestEqual("EXPECT_EQ", C.ToHex(), "#01020304");
}

// =================================================================
// Constants
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FColorTest, Constants)
{
	TestEqual("EXPECT_EQ", FColor::White, FColor(255, 255, 255, 255));
	TestEqual("EXPECT_EQ", FColor::Black, FColor(0, 0, 0, 255));
	TestEqual("EXPECT_EQ", FColor::Red, FColor(255, 0, 0, 255));
	TestEqual("EXPECT_EQ", FColor::Green, FColor(0, 255, 0, 255));
	TestEqual("EXPECT_EQ", FColor::Blue, FColor(0, 0, 255, 255));
	TestEqual("EXPECT_EQ", FColor::Yellow, FColor(255, 255, 0, 255));
	TestEqual("EXPECT_EQ", FColor::Transparent, FColor(0, 0, 0, 0));
}
