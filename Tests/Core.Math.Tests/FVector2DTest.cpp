// Copyright EnigmaEngine. All Rights Reserved.

/// @file FVector2DTest.cpp
/// @brief Unit tests for FVector2D.

#include <gtest/gtest.h>
#include "Math/Vector2D.h"

using Enigma::FVector2D;
using Enigma::FMath;

static constexpr float T = 1e-5f;

// =================================================================
// Constructors
// =================================================================

TEST(FVector2DTest, DefaultConstructor)
{
	constexpr FVector2D V;
	EXPECT_EQ(V.X, 0.0f);
	EXPECT_EQ(V.Y, 0.0f);
}

TEST(FVector2DTest, ComponentConstructor)
{
	constexpr FVector2D V(3.0f, 4.0f);
	EXPECT_EQ(V.X, 3.0f);
	EXPECT_EQ(V.Y, 4.0f);
}

TEST(FVector2DTest, UniformConstructor)
{
	constexpr FVector2D V(5.0f);
	EXPECT_EQ(V.X, 5.0f);
	EXPECT_EQ(V.Y, 5.0f);
}

// =================================================================
// Arithmetic
// =================================================================

TEST(FVector2DTest, Addition)
{
	constexpr FVector2D A(1.0f, 2.0f);
	constexpr FVector2D B(3.0f, 4.0f);
	constexpr FVector2D R = A + B;
	EXPECT_EQ(R.X, 4.0f);
	EXPECT_EQ(R.Y, 6.0f);
}

TEST(FVector2DTest, Subtraction)
{
	constexpr FVector2D A(5.0f, 7.0f);
	constexpr FVector2D B(2.0f, 3.0f);
	constexpr FVector2D R = A - B;
	EXPECT_EQ(R.X, 3.0f);
	EXPECT_EQ(R.Y, 4.0f);
}

TEST(FVector2DTest, ScalarMultiplication)
{
	constexpr FVector2D V(2.0f, 3.0f);
	constexpr FVector2D R = V * 2.0f;
	EXPECT_EQ(R.X, 4.0f);
	EXPECT_EQ(R.Y, 6.0f);
}

TEST(FVector2DTest, ScalarMultiplicationCommutative)
{
	constexpr FVector2D V(2.0f, 3.0f);
	constexpr FVector2D R = 2.0f * V;
	EXPECT_EQ(R.X, 4.0f);
	EXPECT_EQ(R.Y, 6.0f);
}

TEST(FVector2DTest, ScalarDivision)
{
	constexpr FVector2D V(6.0f, 8.0f);
	constexpr FVector2D R = V / 2.0f;
	EXPECT_EQ(R.X, 3.0f);
	EXPECT_EQ(R.Y, 4.0f);
}

TEST(FVector2DTest, UnaryNegation)
{
	constexpr FVector2D V(1.0f, -2.0f);
	constexpr FVector2D R = -V;
	EXPECT_EQ(R.X, -1.0f);
	EXPECT_EQ(R.Y, 2.0f);
}

TEST(FVector2DTest, CompoundAddition)
{
	FVector2D V(1.0f, 2.0f);
	V += FVector2D(3.0f, 4.0f);
	EXPECT_EQ(V.X, 4.0f);
	EXPECT_EQ(V.Y, 6.0f);
}

// =================================================================
// Comparison
// =================================================================

TEST(FVector2DTest, EqualityOperator)
{
	constexpr FVector2D A(1.0f, 2.0f);
	constexpr FVector2D B(1.0f, 2.0f);
	constexpr FVector2D C(1.0f, 3.0f);
	EXPECT_TRUE(A == B);
	EXPECT_TRUE(A != C);
}

// =================================================================
// Size / Normalize
// =================================================================

TEST(FVector2DTest, SizeKnownValues)
{
	const FVector2D V(3.0f, 4.0f);
	EXPECT_NEAR(V.Size(), 5.0f, T);
}

TEST(FVector2DTest, SizeSquared)
{
	constexpr FVector2D V(3.0f, 4.0f);
	EXPECT_EQ(V.SizeSquared(), 25.0f);
}

TEST(FVector2DTest, GetNormalizedUnit)
{
	const FVector2D V(3.0f, 4.0f);
	const FVector2D N = V.GetNormalized();
	EXPECT_NEAR(N.X, 0.6f, T);
	EXPECT_NEAR(N.Y, 0.8f, T);
}

TEST(FVector2DTest, GetNormalizedZeroVector)
{
	const FVector2D V(0.0f, 0.0f);
	const FVector2D N = V.GetNormalized();
	EXPECT_EQ(N.X, 0.0f);
	EXPECT_EQ(N.Y, 0.0f);
}

TEST(FVector2DTest, NormalizeInPlace)
{
	FVector2D V(3.0f, 4.0f);
	EXPECT_TRUE(V.Normalize());
	EXPECT_NEAR(V.Size(), 1.0f, T);
}

TEST(FVector2DTest, NormalizeZeroReturnsFalse)
{
	FVector2D V(0.0f, 0.0f);
	EXPECT_FALSE(V.Normalize());
}

// =================================================================
// DotProduct / CrossProduct / Distance
// =================================================================

TEST(FVector2DTest, DotProduct)
{
	constexpr float D = FVector2D::DotProduct(FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f));
	EXPECT_EQ(D, 0.0f);
}

TEST(FVector2DTest, DotProductParallel)
{
	constexpr float D = FVector2D::DotProduct(FVector2D(2.0f, 3.0f), FVector2D(2.0f, 3.0f));
	EXPECT_EQ(D, 13.0f);
}

TEST(FVector2DTest, CrossProduct)
{
	constexpr float C = FVector2D::CrossProduct(FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f));
	EXPECT_EQ(C, 1.0f);
}

TEST(FVector2DTest, Distance)
{
	const float D = FVector2D::Distance(FVector2D(0.0f, 0.0f), FVector2D(3.0f, 4.0f));
	EXPECT_NEAR(D, 5.0f, T);
}

TEST(FVector2DTest, DistSquared)
{
	constexpr float D = FVector2D::DistSquared(FVector2D(0.0f, 0.0f), FVector2D(3.0f, 4.0f));
	EXPECT_EQ(D, 25.0f);
}

// =================================================================
// Tolerance comparison
// =================================================================

TEST(FVector2DTest, EqualsWithinTolerance)
{
	constexpr FVector2D A(1.0f, 2.0f);
	constexpr FVector2D B(1.00005f, 2.00005f);
	EXPECT_TRUE(A.Equals(B));
}

TEST(FVector2DTest, IsNearlyZero)
{
	constexpr FVector2D V(0.00005f, -0.00005f);
	EXPECT_TRUE(V.IsNearlyZero());
}

TEST(FVector2DTest, IsZero)
{
	EXPECT_TRUE(FVector2D(0.0f, 0.0f).IsZero());
	EXPECT_FALSE(FVector2D(1.0f, 0.0f).IsZero());
}

// =================================================================
// Constants
// =================================================================

TEST(FVector2DTest, PredefinedConstants)
{
	EXPECT_EQ(FVector2D::Zero(), FVector2D(0.0f, 0.0f));
	EXPECT_EQ(FVector2D::One(), FVector2D(1.0f, 1.0f));
	EXPECT_EQ(FVector2D::UnitX(), FVector2D(1.0f, 0.0f));
	EXPECT_EQ(FVector2D::UnitY(), FVector2D(0.0f, 1.0f));
}
