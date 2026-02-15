// Copyright EnigmaEngine. All Rights Reserved.

/// @file FIntVectorTest.cpp
/// @brief Unit tests for FIntVector.

#include <gtest/gtest.h>
#include "Math/IntVector.h"
#include "Math/Vector.h"

using Enigma::FIntVector;
using Enigma::FVector;

// =================================================================
// Constructors
// =================================================================

TEST(FIntVectorTest, DefaultConstructor)
{
	constexpr FIntVector V;
	EXPECT_EQ(V.X, 0);
	EXPECT_EQ(V.Y, 0);
	EXPECT_EQ(V.Z, 0);
}

TEST(FIntVectorTest, ComponentConstructor)
{
	constexpr FIntVector V(1, 2, 3);
	EXPECT_EQ(V.X, 1);
	EXPECT_EQ(V.Y, 2);
	EXPECT_EQ(V.Z, 3);
}

TEST(FIntVectorTest, UniformConstructor)
{
	constexpr FIntVector V(7);
	EXPECT_EQ(V.X, 7);
	EXPECT_EQ(V.Y, 7);
	EXPECT_EQ(V.Z, 7);
}

// =================================================================
// Arithmetic
// =================================================================

TEST(FIntVectorTest, Addition)
{
	constexpr FIntVector R = FIntVector(1, 2, 3) + FIntVector(4, 5, 6);
	EXPECT_EQ(R.X, 5);
	EXPECT_EQ(R.Y, 7);
	EXPECT_EQ(R.Z, 9);
}

TEST(FIntVectorTest, Subtraction)
{
	constexpr FIntVector R = FIntVector(5, 7, 9) - FIntVector(1, 2, 3);
	EXPECT_EQ(R.X, 4);
	EXPECT_EQ(R.Y, 5);
	EXPECT_EQ(R.Z, 6);
}

TEST(FIntVectorTest, ComponentMultiplication)
{
	constexpr FIntVector R = FIntVector(2, 3, 4) * FIntVector(5, 6, 7);
	EXPECT_EQ(R.X, 10);
	EXPECT_EQ(R.Y, 18);
	EXPECT_EQ(R.Z, 28);
}

TEST(FIntVectorTest, ComponentDivision)
{
	constexpr FIntVector R = FIntVector(10, 20, 30) / FIntVector(2, 5, 10);
	EXPECT_EQ(R.X, 5);
	EXPECT_EQ(R.Y, 4);
	EXPECT_EQ(R.Z, 3);
}

TEST(FIntVectorTest, Modulo)
{
	constexpr FIntVector R = FIntVector(10, 7, 15) % FIntVector(3, 4, 6);
	EXPECT_EQ(R.X, 1);
	EXPECT_EQ(R.Y, 3);
	EXPECT_EQ(R.Z, 3);
}

TEST(FIntVectorTest, ModuloNegative)
{
	// C++ truncation semantics: -10 % 3 = -1
	constexpr FIntVector R = FIntVector(-10, -7, 15) % FIntVector(3, 4, 6);
	EXPECT_EQ(R.X, -1);
	EXPECT_EQ(R.Y, -3);
	EXPECT_EQ(R.Z, 3);
}

TEST(FIntVectorTest, ScalarMultiplication)
{
	constexpr FIntVector R = FIntVector(1, 2, 3) * 3;
	EXPECT_EQ(R.X, 3);
	EXPECT_EQ(R.Y, 6);
	EXPECT_EQ(R.Z, 9);
}

TEST(FIntVectorTest, ScalarDivision)
{
	constexpr FIntVector R = FIntVector(9, 6, 3) / 3;
	EXPECT_EQ(R.X, 3);
	EXPECT_EQ(R.Y, 2);
	EXPECT_EQ(R.Z, 1);
}

TEST(FIntVectorTest, UnaryNegation)
{
	constexpr FIntVector R = -FIntVector(1, -2, 3);
	EXPECT_EQ(R.X, -1);
	EXPECT_EQ(R.Y, 2);
	EXPECT_EQ(R.Z, -3);
}

TEST(FIntVectorTest, CompoundAddition)
{
	FIntVector V(1, 2, 3);
	V += FIntVector(4, 5, 6);
	EXPECT_EQ(V.X, 5);
	EXPECT_EQ(V.Y, 7);
	EXPECT_EQ(V.Z, 9);
}

// =================================================================
// Comparison
// =================================================================

TEST(FIntVectorTest, EqualityOperator)
{
	constexpr FIntVector A(1, 2, 3);
	constexpr FIntVector B(1, 2, 3);
	constexpr FIntVector C(1, 2, 4);
	EXPECT_TRUE(A == B);
	EXPECT_TRUE(A != C);
}

TEST(FIntVectorTest, IsZero)
{
	EXPECT_TRUE(FIntVector(0, 0, 0).IsZero());
	EXPECT_FALSE(FIntVector(1, 0, 0).IsZero());
}

// =================================================================
// Direction constants (Y-up coordinate system)
// =================================================================

TEST(FIntVectorTest, DirectionConstants)
{
	EXPECT_EQ(FIntVector::Up(), FIntVector(0, 1, 0));
	EXPECT_EQ(FIntVector::Down(), FIntVector(0, -1, 0));
	EXPECT_EQ(FIntVector::Forward(), FIntVector(0, 0, -1));
	EXPECT_EQ(FIntVector::Backward(), FIntVector(0, 0, 1));
	EXPECT_EQ(FIntVector::Right(), FIntVector(1, 0, 0));
	EXPECT_EQ(FIntVector::Left(), FIntVector(-1, 0, 0));
}

TEST(FIntVectorTest, StaticConstants)
{
	EXPECT_EQ(FIntVector::ZeroValue, FIntVector(0, 0, 0));
	EXPECT_EQ(FIntVector::OneValue, FIntVector(1, 1, 1));
}

// =================================================================
// Float conversion
// =================================================================

TEST(FIntVectorTest, ToFloat)
{
	const FIntVector IV(3, -4, 5);
	const FVector FV = IV.ToFloat();
	EXPECT_EQ(FV.X, 3.0f);
	EXPECT_EQ(FV.Y, -4.0f);
	EXPECT_EQ(FV.Z, 5.0f);
}

TEST(FIntVectorTest, FromFloatPositive)
{
	const FIntVector IV = FIntVector::FromFloat(FVector(1.7f, 2.3f, 3.9f));
	EXPECT_EQ(IV.X, 1);
	EXPECT_EQ(IV.Y, 2);
	EXPECT_EQ(IV.Z, 3);
}

TEST(FIntVectorTest, FromFloatNegativeFloor)
{
	// floor(-1.3) = -2, floor(-2.7) = -3, floor(-0.1) = -1
	const FIntVector IV = FIntVector::FromFloat(FVector(-1.3f, -2.7f, -0.1f));
	EXPECT_EQ(IV.X, -2);
	EXPECT_EQ(IV.Y, -3);
	EXPECT_EQ(IV.Z, -1);
}

TEST(FIntVectorTest, FromFloatExactIntegers)
{
	const FIntVector IV = FIntVector::FromFloat(FVector(5.0f, -3.0f, 0.0f));
	EXPECT_EQ(IV.X, 5);
	EXPECT_EQ(IV.Y, -3);
	EXPECT_EQ(IV.Z, 0);
}

TEST(FIntVectorTest, RoundtripConversion)
{
	const FIntVector Original(10, -20, 30);
	const FIntVector Roundtrip = FIntVector::FromFloat(Original.ToFloat());
	EXPECT_EQ(Original, Roundtrip);
}

// =================================================================
// Edge cases
// =================================================================

TEST(FIntVectorTest, LargeValues)
{
	constexpr FIntVector V(2147483647, -2147483647, 0);  // INT32_MAX
	constexpr FIntVector R = V + FIntVector(0, 0, 1);
	EXPECT_EQ(R.Z, 1);
}

TEST(FIntVectorTest, ConstexprArithmetic)
{
	static_assert(FIntVector(1, 2, 3) + FIntVector(4, 5, 6) == FIntVector(5, 7, 9),
		"FIntVector arithmetic must be constexpr");
}
