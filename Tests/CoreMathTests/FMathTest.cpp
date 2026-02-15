// Copyright EnigmaEngine. All Rights Reserved.

/// @file FMathTest.cpp
/// @brief Unit tests for FMath constants and utility functions.

#include <gtest/gtest.h>
#include "Math/MathUtility.h"

using Enigma::FMath;

// Tolerance for float comparisons
static constexpr float T = 1e-5f;

// =================================================================
// Constants
// =================================================================

TEST(FMathTest, PiConstant)
{
	EXPECT_NEAR(FMath::Pi, 3.14159265f, T);
}

TEST(FMathTest, HalfPiConstant)
{
	EXPECT_NEAR(FMath::HalfPi, FMath::Pi / 2.0f, T);
}

TEST(FMathTest, TwoPiConstant)
{
	EXPECT_NEAR(FMath::TwoPi, FMath::Pi * 2.0f, T);
}

TEST(FMathTest, SmallNumberIsPositive)
{
	EXPECT_GT(FMath::SmallNumber, 0.0f);
	EXPECT_LT(FMath::SmallNumber, 1.0f);
}

TEST(FMathTest, KindaSmallNumberGreaterThanSmallNumber)
{
	EXPECT_GT(FMath::KindaSmallNumber, FMath::SmallNumber);
}

TEST(FMathTest, EpsilonMatchesStdLimits)
{
	EXPECT_EQ(FMath::Epsilon, std::numeric_limits<float>::epsilon());
}

TEST(FMathTest, DegToRadAndRadToDegInverse)
{
	EXPECT_NEAR(FMath::DegToRad * FMath::RadToDeg, 1.0f, T);
}

// =================================================================
// Trigonometry
// =================================================================

TEST(FMathTest, SinKnownValues)
{
	EXPECT_NEAR(FMath::Sin(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::Sin(FMath::HalfPi), 1.0f, T);
	EXPECT_NEAR(FMath::Sin(FMath::Pi), 0.0f, T);
	EXPECT_NEAR(FMath::Sin(FMath::Pi / 6.0f), 0.5f, T);           // sin(30 deg)
	EXPECT_NEAR(FMath::Sin(FMath::Pi / 4.0f), 0.70710678f, T);     // sin(45 deg)
}

TEST(FMathTest, CosKnownValues)
{
	EXPECT_NEAR(FMath::Cos(0.0f), 1.0f, T);
	EXPECT_NEAR(FMath::Cos(FMath::HalfPi), 0.0f, T);
	EXPECT_NEAR(FMath::Cos(FMath::Pi), -1.0f, T);
	EXPECT_NEAR(FMath::Cos(FMath::Pi / 3.0f), 0.5f, T);           // cos(60 deg)
	EXPECT_NEAR(FMath::Cos(FMath::Pi / 4.0f), 0.70710678f, T);     // cos(45 deg)
}

TEST(FMathTest, TanKnownValues)
{
	EXPECT_NEAR(FMath::Tan(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::Tan(FMath::Pi / 4.0f), 1.0f, T);            // tan(45 deg)
	EXPECT_NEAR(FMath::Tan(FMath::Pi), 0.0f, T);                    // tan(180 deg)
}

TEST(FMathTest, AsinKnownValues)
{
	EXPECT_NEAR(FMath::Asin(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::Asin(1.0f), FMath::HalfPi, T);
	EXPECT_NEAR(FMath::Asin(0.5f), FMath::Pi / 6.0f, T);           // asin(0.5) = 30 deg
}

TEST(FMathTest, AcosKnownValues)
{
	EXPECT_NEAR(FMath::Acos(1.0f), 0.0f, T);
	EXPECT_NEAR(FMath::Acos(0.0f), FMath::HalfPi, T);
	EXPECT_NEAR(FMath::Acos(0.5f), FMath::Pi / 3.0f, T);           // acos(0.5) = 60 deg
}

TEST(FMathTest, Atan2KnownValues)
{
	EXPECT_NEAR(FMath::Atan2(0.0f, 1.0f), 0.0f, T);                // (0,1) = 0 deg
	EXPECT_NEAR(FMath::Atan2(1.0f, 0.0f), FMath::HalfPi, T);       // (1,0) = 90 deg
	EXPECT_NEAR(FMath::Atan2(1.0f, 1.0f), FMath::Pi / 4.0f, T);    // (1,1) = 45 deg
	EXPECT_NEAR(FMath::Atan2(-1.0f, 0.0f), -FMath::HalfPi, T);     // (-1,0) = -90 deg
}

// =================================================================
// Common math
// =================================================================

TEST(FMathTest, SqrtKnownValues)
{
	EXPECT_NEAR(FMath::Sqrt(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::Sqrt(1.0f), 1.0f, T);
	EXPECT_NEAR(FMath::Sqrt(4.0f), 2.0f, T);
	EXPECT_NEAR(FMath::Sqrt(2.0f), 1.41421356f, T);
}

TEST(FMathTest, InvSqrtKnownValues)
{
	EXPECT_NEAR(FMath::InvSqrt(1.0f), 1.0f, T);
	EXPECT_NEAR(FMath::InvSqrt(4.0f), 0.5f, T);
}

TEST(FMathTest, FloorAndCeil)
{
	EXPECT_NEAR(FMath::Floor(2.7f), 2.0f, T);
	EXPECT_NEAR(FMath::Floor(-2.3f), -3.0f, T);
	EXPECT_NEAR(FMath::Ceil(2.3f), 3.0f, T);
	EXPECT_NEAR(FMath::Ceil(-2.7f), -2.0f, T);
}

TEST(FMathTest, FmodKnownValues)
{
	EXPECT_NEAR(FMath::Fmod(5.0f, 3.0f), 2.0f, T);
	EXPECT_NEAR(FMath::Fmod(-5.0f, 3.0f), -2.0f, T);
	EXPECT_NEAR(FMath::Fmod(10.0f, 5.0f), 0.0f, T);
}

// =================================================================
// Pure arithmetic (constexpr)
// =================================================================

TEST(FMathTest, AbsPositiveAndNegative)
{
	EXPECT_EQ(FMath::Abs(5.0f), 5.0f);
	EXPECT_EQ(FMath::Abs(-5.0f), 5.0f);
	EXPECT_EQ(FMath::Abs(0.0f), 0.0f);
}

TEST(FMathTest, AbsConstexpr)
{
	static_assert(FMath::Abs(-3.0f) == 3.0f, "Abs must be constexpr");
}

TEST(FMathTest, SquareValues)
{
	EXPECT_EQ(FMath::Square(3.0f), 9.0f);
	EXPECT_EQ(FMath::Square(-4.0f), 16.0f);
	EXPECT_EQ(FMath::Square(0.0f), 0.0f);
}

TEST(FMathTest, MinAndMax)
{
	EXPECT_EQ(FMath::Min(3, 7), 3);
	EXPECT_EQ(FMath::Max(3, 7), 7);
	EXPECT_EQ(FMath::Min(-1.0f, 1.0f), -1.0f);
	EXPECT_EQ(FMath::Max(-1.0f, 1.0f), 1.0f);
}

// =================================================================
// Clamp
// =================================================================

TEST(FMathTest, ClampWithinRange)
{
	EXPECT_EQ(FMath::Clamp(5, 0, 10), 5);
	EXPECT_EQ(FMath::Clamp(5.0f, 0.0f, 10.0f), 5.0f);
}

TEST(FMathTest, ClampBelowMin)
{
	EXPECT_EQ(FMath::Clamp(-5, 0, 10), 0);
	EXPECT_EQ(FMath::Clamp(-1.0f, 0.0f, 1.0f), 0.0f);
}

TEST(FMathTest, ClampAboveMax)
{
	EXPECT_EQ(FMath::Clamp(15, 0, 10), 10);
	EXPECT_EQ(FMath::Clamp(2.0f, 0.0f, 1.0f), 1.0f);
}

TEST(FMathTest, ClampAtBoundaries)
{
	EXPECT_EQ(FMath::Clamp(0, 0, 10), 0);
	EXPECT_EQ(FMath::Clamp(10, 0, 10), 10);
}

TEST(FMathTest, ClampConstexpr)
{
	static_assert(FMath::Clamp(5, 0, 3) == 3, "Clamp must be constexpr");
}

// =================================================================
// Lerp
// =================================================================

TEST(FMathTest, LerpAtAlphaZero)
{
	EXPECT_NEAR(FMath::Lerp(10.0f, 20.0f, 0.0f), 10.0f, T);
}

TEST(FMathTest, LerpAtAlphaOne)
{
	EXPECT_NEAR(FMath::Lerp(10.0f, 20.0f, 1.0f), 20.0f, T);
}

TEST(FMathTest, LerpAtAlphaHalf)
{
	EXPECT_NEAR(FMath::Lerp(10.0f, 20.0f, 0.5f), 15.0f, T);
}

TEST(FMathTest, LerpNegativeRange)
{
	EXPECT_NEAR(FMath::Lerp(-10.0f, 10.0f, 0.5f), 0.0f, T);
}

// =================================================================
// Degree / Radian conversion
// =================================================================

TEST(FMathTest, DegreesToRadiansKnownValues)
{
	EXPECT_NEAR(FMath::DegreesToRadians(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::DegreesToRadians(90.0f), FMath::HalfPi, T);
	EXPECT_NEAR(FMath::DegreesToRadians(180.0f), FMath::Pi, T);
	EXPECT_NEAR(FMath::DegreesToRadians(360.0f), FMath::TwoPi, T);
}

TEST(FMathTest, RadiansToDegreesKnownValues)
{
	EXPECT_NEAR(FMath::RadiansToDegrees(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::RadiansToDegrees(FMath::HalfPi), 90.0f, T);
	EXPECT_NEAR(FMath::RadiansToDegrees(FMath::Pi), 180.0f, T);
	EXPECT_NEAR(FMath::RadiansToDegrees(FMath::TwoPi), 360.0f, T);
}

TEST(FMathTest, DegreeRadianRoundtrip)
{
	const float Angles[] = { 0.0f, 30.0f, 45.0f, 90.0f, 180.0f, 270.0f, 360.0f, -45.0f };
	for (float Deg : Angles)
	{
		EXPECT_NEAR(FMath::RadiansToDegrees(FMath::DegreesToRadians(Deg)), Deg, T);
	}
}

TEST(FMathTest, DegreesToRadiansConstexpr)
{
	static_assert(FMath::DegreesToRadians(180.0f) > 3.14f, "DegreesToRadians must be constexpr");
}

// =================================================================
// IsNearlyEqual / IsNearlyZero
// =================================================================

TEST(FMathTest, IsNearlyEqualExactMatch)
{
	EXPECT_TRUE(FMath::IsNearlyEqual(1.0f, 1.0f));
}

TEST(FMathTest, IsNearlyEqualWithinTolerance)
{
	EXPECT_TRUE(FMath::IsNearlyEqual(1.0f, 1.0f + FMath::SmallNumber * 0.5f));
}

TEST(FMathTest, IsNearlyEqualBeyondTolerance)
{
	EXPECT_FALSE(FMath::IsNearlyEqual(1.0f, 1.001f));
	EXPECT_FALSE(FMath::IsNearlyEqual(0.0f, FMath::SmallNumber * 2.0f));
}

TEST(FMathTest, IsNearlyEqualCustomTolerance)
{
	EXPECT_TRUE(FMath::IsNearlyEqual(1.0f, 1.05f, 0.1f));
	EXPECT_FALSE(FMath::IsNearlyEqual(1.0f, 1.2f, 0.1f));
}

TEST(FMathTest, IsNearlyZeroExactZero)
{
	EXPECT_TRUE(FMath::IsNearlyZero(0.0f));
}

TEST(FMathTest, IsNearlyZeroWithinTolerance)
{
	EXPECT_TRUE(FMath::IsNearlyZero(FMath::SmallNumber * 0.5f));
}

TEST(FMathTest, IsNearlyZeroBeyondTolerance)
{
	EXPECT_FALSE(FMath::IsNearlyZero(FMath::SmallNumber * 2.0f));
}

TEST(FMathTest, IsNearlyZeroNegative)
{
	EXPECT_TRUE(FMath::IsNearlyZero(-FMath::SmallNumber * 0.5f));
	EXPECT_FALSE(FMath::IsNearlyZero(-1.0f));
}

TEST(FMathTest, IsNearlyEqualConstexpr)
{
	static_assert(FMath::IsNearlyEqual(1.0f, 1.0f), "IsNearlyEqual must be constexpr");
	static_assert(FMath::IsNearlyZero(0.0f), "IsNearlyZero must be constexpr");
}

// =================================================================
// Angle normalization
// =================================================================

TEST(FMathTest, NormalizeAngleAlreadyNormalized)
{
	EXPECT_NEAR(FMath::NormalizeAngle(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(90.0f), 90.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(-90.0f), -90.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(179.0f), 179.0f, T);
}

TEST(FMathTest, NormalizeAngleWrapsPositive)
{
	EXPECT_NEAR(FMath::NormalizeAngle(270.0f), -90.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(360.0f), 0.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(540.0f), -180.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(720.0f), 0.0f, T);
}

TEST(FMathTest, NormalizeAngleWrapsNegative)
{
	EXPECT_NEAR(FMath::NormalizeAngle(-270.0f), 90.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(-360.0f), 0.0f, T);
	EXPECT_NEAR(FMath::NormalizeAngle(-540.0f), -180.0f, T);
}

TEST(FMathTest, ClampAngleToZero360)
{
	EXPECT_NEAR(FMath::ClampAngle(0.0f), 0.0f, T);
	EXPECT_NEAR(FMath::ClampAngle(90.0f), 90.0f, T);
	EXPECT_NEAR(FMath::ClampAngle(-90.0f), 270.0f, T);
	EXPECT_NEAR(FMath::ClampAngle(360.0f), 0.0f, T);
	EXPECT_NEAR(FMath::ClampAngle(450.0f), 90.0f, T);
}
