// Copyright EnigmaEngine. All Rights Reserved.

/// @file FVector4Test.cpp
/// @brief Unit tests for FVector4.

#include <gtest/gtest.h>
#include "Math/Vector4.h"

using Enigma::FVector;
using Enigma::FVector4;
using Enigma::FMath;

static constexpr float T = 1e-5f;

// =================================================================
// Constructors
// =================================================================

TEST(FVector4Test, DefaultConstructor)
{
	constexpr FVector4 V;
	EXPECT_EQ(V.X, 0.0f);
	EXPECT_EQ(V.Y, 0.0f);
	EXPECT_EQ(V.Z, 0.0f);
	EXPECT_EQ(V.W, 0.0f);
}

TEST(FVector4Test, ComponentConstructor)
{
	constexpr FVector4 V(1.0f, 2.0f, 3.0f, 4.0f);
	EXPECT_EQ(V.X, 1.0f);
	EXPECT_EQ(V.Y, 2.0f);
	EXPECT_EQ(V.Z, 3.0f);
	EXPECT_EQ(V.W, 4.0f);
}

TEST(FVector4Test, FromVectorDefaultW)
{
	constexpr FVector V3(1.0f, 2.0f, 3.0f);
	constexpr FVector4 V4(V3);
	EXPECT_EQ(V4.X, 1.0f);
	EXPECT_EQ(V4.Y, 2.0f);
	EXPECT_EQ(V4.Z, 3.0f);
	EXPECT_EQ(V4.W, 1.0f);  // point: W=1
}

TEST(FVector4Test, FromVectorDirectionW)
{
	constexpr FVector V3(1.0f, 0.0f, 0.0f);
	constexpr FVector4 V4(V3, 0.0f);
	EXPECT_EQ(V4.W, 0.0f);  // direction: W=0
}

// =================================================================
// Arithmetic
// =================================================================

TEST(FVector4Test, Addition)
{
	constexpr FVector4 R = FVector4(1.0f, 2.0f, 3.0f, 4.0f) + FVector4(5.0f, 6.0f, 7.0f, 8.0f);
	EXPECT_EQ(R.X, 6.0f);
	EXPECT_EQ(R.Y, 8.0f);
	EXPECT_EQ(R.Z, 10.0f);
	EXPECT_EQ(R.W, 12.0f);
}

TEST(FVector4Test, Subtraction)
{
	constexpr FVector4 R = FVector4(5.0f, 6.0f, 7.0f, 8.0f) - FVector4(1.0f, 2.0f, 3.0f, 4.0f);
	EXPECT_EQ(R.X, 4.0f);
	EXPECT_EQ(R.Y, 4.0f);
	EXPECT_EQ(R.Z, 4.0f);
	EXPECT_EQ(R.W, 4.0f);
}

TEST(FVector4Test, ScalarMultiplication)
{
	constexpr FVector4 R = FVector4(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f;
	EXPECT_EQ(R.X, 2.0f);
	EXPECT_EQ(R.Y, 4.0f);
	EXPECT_EQ(R.Z, 6.0f);
	EXPECT_EQ(R.W, 8.0f);
}

TEST(FVector4Test, ScalarMultiplicationCommutative)
{
	constexpr FVector4 R = 3.0f * FVector4(1.0f, 2.0f, 3.0f, 4.0f);
	EXPECT_EQ(R.X, 3.0f);
	EXPECT_EQ(R.Y, 6.0f);
	EXPECT_EQ(R.Z, 9.0f);
	EXPECT_EQ(R.W, 12.0f);
}

TEST(FVector4Test, UnaryNegation)
{
	constexpr FVector4 R = -FVector4(1.0f, -2.0f, 3.0f, -4.0f);
	EXPECT_EQ(R.X, -1.0f);
	EXPECT_EQ(R.Y, 2.0f);
	EXPECT_EQ(R.Z, -3.0f);
	EXPECT_EQ(R.W, 4.0f);
}

// =================================================================
// Comparison
// =================================================================

TEST(FVector4Test, EqualityOperator)
{
	constexpr FVector4 A(1.0f, 2.0f, 3.0f, 4.0f);
	constexpr FVector4 B(1.0f, 2.0f, 3.0f, 4.0f);
	constexpr FVector4 C(1.0f, 2.0f, 3.0f, 5.0f);
	EXPECT_TRUE(A == B);
	EXPECT_TRUE(A != C);
}

// =================================================================
// Size
// =================================================================

TEST(FVector4Test, SizeSquared)
{
	constexpr FVector4 V(1.0f, 2.0f, 3.0f, 4.0f);
	EXPECT_EQ(V.SizeSquared(), 30.0f);
}

TEST(FVector4Test, Size)
{
	const FVector4 V(2.0f, 0.0f, 0.0f, 0.0f);
	EXPECT_NEAR(V.Size(), 2.0f, T);
}

TEST(FVector4Test, Size3IgnoresW)
{
	const FVector4 V(1.0f, 2.0f, 2.0f, 100.0f);
	EXPECT_NEAR(V.Size3(), 3.0f, T);
}

TEST(FVector4Test, SizeSquared3IgnoresW)
{
	constexpr FVector4 V(1.0f, 2.0f, 2.0f, 100.0f);
	EXPECT_EQ(V.SizeSquared3(), 9.0f);
}

// =================================================================
// DotProduct
// =================================================================

TEST(FVector4Test, DotProduct4D)
{
	constexpr float D = FVector4::DotProduct(
		FVector4(1.0f, 2.0f, 3.0f, 4.0f),
		FVector4(5.0f, 6.0f, 7.0f, 8.0f)
	);
	// 5 + 12 + 21 + 32 = 70
	EXPECT_EQ(D, 70.0f);
}

TEST(FVector4Test, DotProduct3DIgnoresW)
{
	constexpr float D = FVector4::DotProduct3(
		FVector4(1.0f, 2.0f, 3.0f, 100.0f),
		FVector4(4.0f, 5.0f, 6.0f, 200.0f)
	);
	// 4 + 10 + 18 = 32
	EXPECT_EQ(D, 32.0f);
}

// =================================================================
// Tolerance comparison
// =================================================================

TEST(FVector4Test, EqualsWithinTolerance)
{
	constexpr FVector4 A(1.0f, 2.0f, 3.0f, 4.0f);
	constexpr FVector4 B(1.00005f, 2.00005f, 3.00005f, 4.00005f);
	EXPECT_TRUE(A.Equals(B));
}

TEST(FVector4Test, IsNearlyZero)
{
	constexpr FVector4 V(0.00005f, -0.00005f, 0.00001f, -0.00001f);
	EXPECT_TRUE(V.IsNearlyZero());
}

// =================================================================
// Edge cases
// =================================================================

TEST(FVector4Test, FromVectorCustomW)
{
	constexpr FVector V3(10.0f, 20.0f, 30.0f);
	constexpr FVector4 V4(V3, 0.5f);
	EXPECT_EQ(V4.X, 10.0f);
	EXPECT_EQ(V4.Y, 20.0f);
	EXPECT_EQ(V4.Z, 30.0f);
	EXPECT_EQ(V4.W, 0.5f);
}

TEST(FVector4Test, LargeValues)
{
	constexpr FVector4 V(1e10f, 1e10f, 1e10f, 1e10f);
	EXPECT_GT(V.SizeSquared(), 0.0f);
}
