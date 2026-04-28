// Copyright EnigmaEngine. All Rights Reserved.

/// @file FMathTest.cpp
/// @brief Unit tests for FMath constants and utility functions.

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
#include "Math/MathUtility.h"

using Enigma::FMath;

// Tolerance for float comparisons
static constexpr float T = 1e-5f;

// =================================================================
// Constants
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, PiConstant)
{
	TestNear("EXPECT_NEAR", FMath::Pi, 3.14159265f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, HalfPiConstant)
{
	TestNear("EXPECT_NEAR", FMath::HalfPi, FMath::Pi / 2.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, TwoPiConstant)
{
	TestNear("EXPECT_NEAR", FMath::TwoPi, FMath::Pi * 2.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, SmallNumberIsPositive)
{
	TestGreaterThan("EXPECT_GT", FMath::SmallNumber, 0.0f);
	TestLessThan("EXPECT_LT", FMath::SmallNumber, 1.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, KindaSmallNumberGreaterThanSmallNumber)
{
	TestGreaterThan("EXPECT_GT", FMath::KindaSmallNumber, FMath::SmallNumber);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, EpsilonMatchesStdLimits)
{
	TestEqual("EXPECT_EQ", FMath::Epsilon, std::numeric_limits<float>::epsilon());
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, DegToRadAndRadToDegInverse)
{
	TestNear("EXPECT_NEAR", FMath::DegToRad * FMath::RadToDeg, 1.0f, T);
}

// =================================================================
// Trigonometry
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, SinKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Sin(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::Sin(FMath::HalfPi), 1.0f, T);
	TestNear("EXPECT_NEAR", FMath::Sin(FMath::Pi), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::Sin(FMath::Pi / 6.0f), 0.5f, T); // sin(30 deg)
	TestNear("EXPECT_NEAR", FMath::Sin(FMath::Pi / 4.0f), 0.70710678f, T); // sin(45 deg)
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, CosKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Cos(0.0f), 1.0f, T);
	TestNear("EXPECT_NEAR", FMath::Cos(FMath::HalfPi), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::Cos(FMath::Pi), -1.0f, T);
	TestNear("EXPECT_NEAR", FMath::Cos(FMath::Pi / 3.0f), 0.5f, T); // cos(60 deg)
	TestNear("EXPECT_NEAR", FMath::Cos(FMath::Pi / 4.0f), 0.70710678f, T); // cos(45 deg)
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, TanKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Tan(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::Tan(FMath::Pi / 4.0f), 1.0f, T); // tan(45 deg)
	TestNear("EXPECT_NEAR", FMath::Tan(FMath::Pi), 0.0f, T); // tan(180 deg)
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, AsinKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Asin(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::Asin(1.0f), FMath::HalfPi, T);
	TestNear("EXPECT_NEAR", FMath::Asin(0.5f), FMath::Pi / 6.0f, T); // asin(0.5) = 30 deg
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, AcosKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Acos(1.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::Acos(0.0f), FMath::HalfPi, T);
	TestNear("EXPECT_NEAR", FMath::Acos(0.5f), FMath::Pi / 3.0f, T); // acos(0.5) = 60 deg
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, Atan2KnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Atan2(0.0f, 1.0f), 0.0f, T); // (0,1) = 0 deg
	TestNear("EXPECT_NEAR", FMath::Atan2(1.0f, 0.0f), FMath::HalfPi, T); // (1,0) = 90 deg
	TestNear("EXPECT_NEAR", FMath::Atan2(1.0f, 1.0f), FMath::Pi / 4.0f, T); // (1,1) = 45 deg
	TestNear("EXPECT_NEAR", FMath::Atan2(-1.0f, 0.0f), -FMath::HalfPi, T); // (-1,0) = -90 deg
}

// =================================================================
// Common math
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, SqrtKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Sqrt(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::Sqrt(1.0f), 1.0f, T);
	TestNear("EXPECT_NEAR", FMath::Sqrt(4.0f), 2.0f, T);
	TestNear("EXPECT_NEAR", FMath::Sqrt(2.0f), 1.41421356f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, InvSqrtKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::InvSqrt(1.0f), 1.0f, T);
	TestNear("EXPECT_NEAR", FMath::InvSqrt(4.0f), 0.5f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, FloorAndCeil)
{
	TestNear("EXPECT_NEAR", FMath::Floor(2.7f), 2.0f, T);
	TestNear("EXPECT_NEAR", FMath::Floor(-2.3f), -3.0f, T);
	TestNear("EXPECT_NEAR", FMath::Ceil(2.3f), 3.0f, T);
	TestNear("EXPECT_NEAR", FMath::Ceil(-2.7f), -2.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, FmodKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::Fmod(5.0f, 3.0f), 2.0f, T);
	TestNear("EXPECT_NEAR", FMath::Fmod(-5.0f, 3.0f), -2.0f, T);
	TestNear("EXPECT_NEAR", FMath::Fmod(10.0f, 5.0f), 0.0f, T);
}

// =================================================================
// Pure arithmetic (constexpr)
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, AbsPositiveAndNegative)
{
	TestEqual("EXPECT_EQ", FMath::Abs(5.0f), 5.0f);
	TestEqual("EXPECT_EQ", FMath::Abs(-5.0f), 5.0f);
	TestEqual("EXPECT_EQ", FMath::Abs(0.0f), 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, AbsConstexpr)
{
	static_assert(FMath::Abs(-3.0f) == 3.0f, "Abs must be constexpr");
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, SquareValues)
{
	TestEqual("EXPECT_EQ", FMath::Square(3.0f), 9.0f);
	TestEqual("EXPECT_EQ", FMath::Square(-4.0f), 16.0f);
	TestEqual("EXPECT_EQ", FMath::Square(0.0f), 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, MinAndMax)
{
	TestEqual("EXPECT_EQ", FMath::Min(3, 7), 3);
	TestEqual("EXPECT_EQ", FMath::Max(3, 7), 7);
	TestEqual("EXPECT_EQ", FMath::Min(-1.0f, 1.0f), -1.0f);
	TestEqual("EXPECT_EQ", FMath::Max(-1.0f, 1.0f), 1.0f);
}

// =================================================================
// Clamp
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, ClampWithinRange)
{
	TestEqual("EXPECT_EQ", FMath::Clamp(5, 0, 10), 5);
	TestEqual("EXPECT_EQ", FMath::Clamp(5.0f, 0.0f, 10.0f), 5.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, ClampBelowMin)
{
	TestEqual("EXPECT_EQ", FMath::Clamp(-5, 0, 10), 0);
	TestEqual("EXPECT_EQ", FMath::Clamp(-1.0f, 0.0f, 1.0f), 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, ClampAboveMax)
{
	TestEqual("EXPECT_EQ", FMath::Clamp(15, 0, 10), 10);
	TestEqual("EXPECT_EQ", FMath::Clamp(2.0f, 0.0f, 1.0f), 1.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, ClampAtBoundaries)
{
	TestEqual("EXPECT_EQ", FMath::Clamp(0, 0, 10), 0);
	TestEqual("EXPECT_EQ", FMath::Clamp(10, 0, 10), 10);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, ClampConstexpr)
{
	static_assert(FMath::Clamp(5, 0, 3) == 3, "Clamp must be constexpr");
}

// =================================================================
// Lerp
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, LerpAtAlphaZero)
{
	TestNear("EXPECT_NEAR", FMath::Lerp(10.0f, 20.0f, 0.0f), 10.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, LerpAtAlphaOne)
{
	TestNear("EXPECT_NEAR", FMath::Lerp(10.0f, 20.0f, 1.0f), 20.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, LerpAtAlphaHalf)
{
	TestNear("EXPECT_NEAR", FMath::Lerp(10.0f, 20.0f, 0.5f), 15.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, LerpNegativeRange)
{
	TestNear("EXPECT_NEAR", FMath::Lerp(-10.0f, 10.0f, 0.5f), 0.0f, T);
}

// =================================================================
// Degree / Radian conversion
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, DegreesToRadiansKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::DegreesToRadians(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::DegreesToRadians(90.0f), FMath::HalfPi, T);
	TestNear("EXPECT_NEAR", FMath::DegreesToRadians(180.0f), FMath::Pi, T);
	TestNear("EXPECT_NEAR", FMath::DegreesToRadians(360.0f), FMath::TwoPi, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, RadiansToDegreesKnownValues)
{
	TestNear("EXPECT_NEAR", FMath::RadiansToDegrees(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::RadiansToDegrees(FMath::HalfPi), 90.0f, T);
	TestNear("EXPECT_NEAR", FMath::RadiansToDegrees(FMath::Pi), 180.0f, T);
	TestNear("EXPECT_NEAR", FMath::RadiansToDegrees(FMath::TwoPi), 360.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, DegreeRadianRoundtrip)
{
	const float Angles[] = { 0.0f, 30.0f, 45.0f, 90.0f, 180.0f, 270.0f, 360.0f, -45.0f };
	for (float Deg : Angles)
	{
		TestNear("EXPECT_NEAR", FMath::RadiansToDegrees(FMath::DegreesToRadians(Deg)), Deg, T);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, DegreesToRadiansConstexpr)
{
	static_assert(FMath::DegreesToRadians(180.0f) > 3.14f, "DegreesToRadians must be constexpr");
}

// =================================================================
// IsNearlyEqual / IsNearlyZero
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyEqualExactMatch)
{
	TestTrue("EXPECT_TRUE", FMath::IsNearlyEqual(1.0f, 1.0f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyEqualWithinTolerance)
{
	TestTrue("EXPECT_TRUE", FMath::IsNearlyEqual(1.0f, 1.0f + FMath::SmallNumber * 0.5f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyEqualBeyondTolerance)
{
	TestFalse("EXPECT_FALSE", FMath::IsNearlyEqual(1.0f, 1.001f));
	TestFalse("EXPECT_FALSE", FMath::IsNearlyEqual(0.0f, FMath::SmallNumber * 2.0f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyEqualCustomTolerance)
{
	TestTrue("EXPECT_TRUE", FMath::IsNearlyEqual(1.0f, 1.05f, 0.1f));
	TestFalse("EXPECT_FALSE", FMath::IsNearlyEqual(1.0f, 1.2f, 0.1f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyZeroExactZero)
{
	TestTrue("EXPECT_TRUE", FMath::IsNearlyZero(0.0f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyZeroWithinTolerance)
{
	TestTrue("EXPECT_TRUE", FMath::IsNearlyZero(FMath::SmallNumber * 0.5f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyZeroBeyondTolerance)
{
	TestFalse("EXPECT_FALSE", FMath::IsNearlyZero(FMath::SmallNumber * 2.0f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyZeroNegative)
{
	TestTrue("EXPECT_TRUE", FMath::IsNearlyZero(-FMath::SmallNumber * 0.5f));
	TestFalse("EXPECT_FALSE", FMath::IsNearlyZero(-1.0f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, IsNearlyEqualConstexpr)
{
	static_assert(FMath::IsNearlyEqual(1.0f, 1.0f), "IsNearlyEqual must be constexpr");
	static_assert(FMath::IsNearlyZero(0.0f), "IsNearlyZero must be constexpr");
}

// =================================================================
// Angle normalization
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, NormalizeAngleAlreadyNormalized)
{
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(90.0f), 90.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(-90.0f), -90.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(179.0f), 179.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, NormalizeAngleWrapsPositive)
{
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(270.0f), -90.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(360.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(540.0f), -180.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(720.0f), 0.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, NormalizeAngleWrapsNegative)
{
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(-270.0f), 90.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(-360.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::NormalizeAngle(-540.0f), -180.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathTest, ClampAngleToZero360)
{
	TestNear("EXPECT_NEAR", FMath::ClampAngle(0.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::ClampAngle(90.0f), 90.0f, T);
	TestNear("EXPECT_NEAR", FMath::ClampAngle(-90.0f), 270.0f, T);
	TestNear("EXPECT_NEAR", FMath::ClampAngle(360.0f), 0.0f, T);
	TestNear("EXPECT_NEAR", FMath::ClampAngle(450.0f), 90.0f, T);
}
