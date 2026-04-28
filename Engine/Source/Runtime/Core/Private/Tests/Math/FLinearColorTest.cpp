// Copyright EnigmaEngine. All Rights Reserved.

/// @file FLinearColorTest.cpp
/// @brief Unit tests for FLinearColor.

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
#include "Math/LinearColor.h"
#include "Math/Color.h"

using Enigma::FLinearColor;
using Enigma::FColor;
using Enigma::FMath;

static constexpr float T = 1e-4f;

// =================================================================
// Constructors
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, DefaultConstructor)
{
	constexpr FLinearColor C;
	TestEqual("EXPECT_EQ", C.R, 0.0f);
	TestEqual("EXPECT_EQ", C.G, 0.0f);
	TestEqual("EXPECT_EQ", C.B, 0.0f);
	TestEqual("EXPECT_EQ", C.A, 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ComponentConstructor)
{
	constexpr FLinearColor C(0.1f, 0.2f, 0.3f, 0.4f);
	TestEqual("EXPECT_EQ", C.R, 0.1f);
	TestEqual("EXPECT_EQ", C.G, 0.2f);
	TestEqual("EXPECT_EQ", C.B, 0.3f);
	TestEqual("EXPECT_EQ", C.A, 0.4f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ComponentConstructorDefaultAlpha)
{
	constexpr FLinearColor C(0.5f, 0.5f, 0.5f);
	TestEqual("EXPECT_EQ", C.A, 1.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, FromFColor)
{
	// Pure white sRGB should map to linear 1.0.
	const FLinearColor C(FColor(255, 255, 255, 255));
	TestNear("EXPECT_NEAR", C.R, 1.0f, T);
	TestNear("EXPECT_NEAR", C.G, 1.0f, T);
	TestNear("EXPECT_NEAR", C.B, 1.0f, T);
	TestNear("EXPECT_NEAR", C.A, 1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, FromFColorBlack)
{
	const FLinearColor C(FColor(0, 0, 0, 255));
	TestNear("EXPECT_NEAR", C.R, 0.0f, T);
	TestNear("EXPECT_NEAR", C.G, 0.0f, T);
	TestNear("EXPECT_NEAR", C.B, 0.0f, T);
	TestNear("EXPECT_NEAR", C.A, 1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, FromFColorMidGray)
{
	// sRGB 128 ~= linear 0.2158 (standard sRGB curve).
	const FLinearColor C(FColor(128, 128, 128, 255));
	TestNear("EXPECT_NEAR", C.R, 0.2158f, 0.01f);
	TestNear("EXPECT_NEAR", C.G, 0.2158f, 0.01f);
}

// =================================================================
// Arithmetic
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, Addition)
{
	constexpr FLinearColor A(0.1f, 0.2f, 0.3f, 0.4f);
	constexpr FLinearColor B(0.5f, 0.3f, 0.2f, 0.1f);
	constexpr FLinearColor R = A + B;
	TestNear("EXPECT_NEAR", R.R, 0.6f, T);
	TestNear("EXPECT_NEAR", R.G, 0.5f, T);
	TestNear("EXPECT_NEAR", R.B, 0.5f, T);
	TestNear("EXPECT_NEAR", R.A, 0.5f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, Subtraction)
{
	constexpr FLinearColor A(0.5f, 0.5f, 0.5f, 1.0f);
	constexpr FLinearColor B(0.1f, 0.2f, 0.3f, 0.4f);
	constexpr FLinearColor R = A - B;
	TestNear("EXPECT_NEAR", R.R, 0.4f, T);
	TestNear("EXPECT_NEAR", R.G, 0.3f, T);
	TestNear("EXPECT_NEAR", R.B, 0.2f, T);
	TestNear("EXPECT_NEAR", R.A, 0.6f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ScalarMultiply)
{
	constexpr FLinearColor C(0.2f, 0.4f, 0.6f, 1.0f);
	constexpr FLinearColor R = C * 2.0f;
	TestNear("EXPECT_NEAR", R.R, 0.4f, T);
	TestNear("EXPECT_NEAR", R.G, 0.8f, T);
	TestNear("EXPECT_NEAR", R.B, 1.2f, T);
	TestNear("EXPECT_NEAR", R.A, 2.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ScalarMultiplyCommutative)
{
	constexpr FLinearColor C(0.5f, 0.5f, 0.5f, 1.0f);
	constexpr FLinearColor R = 0.5f * C;
	TestNear("EXPECT_NEAR", R.R, 0.25f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ColorMultiply)
{
	constexpr FLinearColor A(0.5f, 1.0f, 0.0f, 1.0f);
	constexpr FLinearColor B(0.5f, 0.5f, 1.0f, 0.5f);
	constexpr FLinearColor R = A * B;
	TestNear("EXPECT_NEAR", R.R, 0.25f, T);
	TestNear("EXPECT_NEAR", R.G, 0.5f, T);
	TestNear("EXPECT_NEAR", R.B, 0.0f, T);
	TestNear("EXPECT_NEAR", R.A, 0.5f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, CompoundAddition)
{
	FLinearColor C(0.1f, 0.2f, 0.3f, 0.4f);
	C += FLinearColor(0.1f, 0.1f, 0.1f, 0.1f);
	TestNear("EXPECT_NEAR", C.R, 0.2f, T);
	TestNear("EXPECT_NEAR", C.G, 0.3f, T);
}

// =================================================================
// Comparison
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, EqualityOperator)
{
	constexpr FLinearColor A(1.0f, 0.0f, 0.0f, 1.0f);
	constexpr FLinearColor B(1.0f, 0.0f, 0.0f, 1.0f);
	TestTrue("EXPECT_TRUE", A == B);
	TestFalse("EXPECT_FALSE", A != B);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, EqualsWithTolerance)
{
	constexpr FLinearColor A(1.0f, 0.0f, 0.0f, 1.0f);
	constexpr FLinearColor B(1.00005f, 0.00005f, 0.0f, 1.0f);
	TestTrue("EXPECT_TRUE", A.Equals(B));
}

// =================================================================
// Conversion: ToFColor
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ToFColorWhite)
{
	const FColor C = FLinearColor::White.ToFColor(true);
	TestEqual("EXPECT_EQ", C.R, 255);
	TestEqual("EXPECT_EQ", C.G, 255);
	TestEqual("EXPECT_EQ", C.B, 255);
	TestEqual("EXPECT_EQ", C.A, 255);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ToFColorBlack)
{
	const FColor C = FLinearColor::Black.ToFColor(true);
	TestEqual("EXPECT_EQ", C.R, 0);
	TestEqual("EXPECT_EQ", C.G, 0);
	TestEqual("EXPECT_EQ", C.B, 0);
	TestEqual("EXPECT_EQ", C.A, 255);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ToFColorClampsNegative)
{
	const FLinearColor Neg(-0.5f, -1.0f, 0.5f, 1.0f);
	const FColor C = Neg.ToFColor(true);
	TestEqual("EXPECT_EQ", C.R, 0);
	TestEqual("EXPECT_EQ", C.G, 0);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ToFColorClampsAboveOne)
{
	const FLinearColor Over(2.0f, 1.5f, 0.5f, 1.0f);
	const FColor C = Over.ToFColor(true);
	TestEqual("EXPECT_EQ", C.R, 255);
	TestEqual("EXPECT_EQ", C.G, 255);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, ToFColorNoSRGB)
{
	// Without sRGB, linear 0.5 should map to ~128.
	const FLinearColor L(0.5f, 0.5f, 0.5f, 1.0f);
	const FColor C = L.ToFColor(false);
	TestNear("EXPECT_NEAR", static_cast<int>(C.R), 128, 1);
	TestNear("EXPECT_NEAR", static_cast<int>(C.G), 128, 1);
}

// =================================================================
// sRGB roundtrip
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, SRGBRoundtripWhite)
{
	const FColor Original(255, 255, 255, 255);
	const FLinearColor Linear(Original);
	const FColor Back = Linear.ToFColor(true);
	TestEqual("EXPECT_EQ", Back.R, Original.R);
	TestEqual("EXPECT_EQ", Back.G, Original.G);
	TestEqual("EXPECT_EQ", Back.B, Original.B);
	TestEqual("EXPECT_EQ", Back.A, Original.A);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, SRGBRoundtripMidValues)
{
	// Test several mid-range values. Allow +/-1 for quantization.
	const uint8_t Values[] = {0, 32, 64, 128, 192, 255};
	for (uint8_t V : Values)
	{
		const FColor Original(V, V, V, 255);
		const FLinearColor Linear(Original);
		const FColor Back = Linear.ToFColor(true);
		TestNear("EXPECT_NEAR", static_cast<int>(Back.R), static_cast<int>(V), 1);
	}
}

// =================================================================
// Lerp
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, LerpAlpha0)
{
	constexpr FLinearColor Black(0.0f, 0.0f, 0.0f, 1.0f);
	constexpr FLinearColor White(1.0f, 1.0f, 1.0f, 1.0f);
	constexpr FLinearColor R = FLinearColor::Lerp(Black, White, 0.0f);
	TestNear("EXPECT_NEAR", R.R, 0.0f, T);
	TestNear("EXPECT_NEAR", R.G, 0.0f, T);
	TestNear("EXPECT_NEAR", R.B, 0.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, LerpAlpha1)
{
	constexpr FLinearColor Black(0.0f, 0.0f, 0.0f, 1.0f);
	constexpr FLinearColor White(1.0f, 1.0f, 1.0f, 1.0f);
	constexpr FLinearColor R = FLinearColor::Lerp(Black, White, 1.0f);
	TestNear("EXPECT_NEAR", R.R, 1.0f, T);
	TestNear("EXPECT_NEAR", R.G, 1.0f, T);
	TestNear("EXPECT_NEAR", R.B, 1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, LerpMidpoint)
{
	constexpr FLinearColor R = FLinearColor::Lerp(
		FLinearColor(0.0f, 0.0f, 0.0f, 1.0f),
		FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), 0.5f);
	TestNear("EXPECT_NEAR", R.R, 0.5f, T);
	TestNear("EXPECT_NEAR", R.G, 0.5f, T);
	TestNear("EXPECT_NEAR", R.B, 0.5f, T);
	TestNear("EXPECT_NEAR", R.A, 1.0f, T);
}

// =================================================================
// Constants
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FLinearColorTest, Constants)
{
	TestTrue("EXPECT_TRUE", FLinearColor::White.Equals(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
	TestTrue("EXPECT_TRUE", FLinearColor::Black.Equals(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));
	TestTrue("EXPECT_TRUE", FLinearColor::Red.Equals(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f)));
	TestTrue("EXPECT_TRUE", FLinearColor::Green.Equals(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)));
	TestTrue("EXPECT_TRUE", FLinearColor::Blue.Equals(FLinearColor(0.0f, 0.0f, 1.0f, 1.0f)));
	TestTrue("EXPECT_TRUE", FLinearColor::Yellow.Equals(FLinearColor(1.0f, 1.0f, 0.0f, 1.0f)));
	TestTrue("EXPECT_TRUE", FLinearColor::Transparent.Equals(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)));
}
