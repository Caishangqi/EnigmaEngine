// Copyright EnigmaEngine. All Rights Reserved.

/// @file RandomStreamTest.cpp
/// @brief Unit tests for FRandomStream and FMath random methods.

#include <gtest/gtest.h>
#include "Math/RandomStream.h"
#include "Math/MathUtility.h"
#include "Math/Vector.h"

#include <cmath>

using Enigma::FRandomStream;
using Enigma::FMath;
using Enigma::FVector;

// =================================================================
// FRandomStream — Determinism
// =================================================================

TEST(FRandomStreamTest, SameSeedProducesSameSequence)
{
	FRandomStream a(42);
	FRandomStream b(42);

	for (int i = 0; i < 100; ++i)
	{
		EXPECT_EQ(a.GetUnsignedInt(), b.GetUnsignedInt());
	}
}

TEST(FRandomStreamTest, SameSeedProducesSameFractions)
{
	FRandomStream a(123);
	FRandomStream b(123);

	for (int i = 0; i < 100; ++i)
	{
		EXPECT_FLOAT_EQ(a.GetFraction(), b.GetFraction());
	}
}

TEST(FRandomStreamTest, SameSeedProducesSameRandRange)
{
	FRandomStream a(7);
	FRandomStream b(7);

	for (int i = 0; i < 100; ++i)
	{
		EXPECT_EQ(a.RandRange(0, 1000), b.RandRange(0, 1000));
	}
}
// =================================================================
// FRandomStream — Reset
// =================================================================

TEST(FRandomStreamTest, ResetReproducesSequence)
{
	FRandomStream s(99);

	// Generate first sequence
	float v0 = s.GetFraction();
	float v1 = s.GetFraction();
	float v2 = s.GetFraction();

	// Reset and verify same sequence
	s.Reset();
	EXPECT_FLOAT_EQ(s.GetFraction(), v0);
	EXPECT_FLOAT_EQ(s.GetFraction(), v1);
	EXPECT_FLOAT_EQ(s.GetFraction(), v2);
}

// =================================================================
// FRandomStream — GenerateNewSeed
// =================================================================

TEST(FRandomStreamTest, GenerateNewSeedChangesSeed)
{
	FRandomStream s(42);
	int32_t oldSeed = s.GetInitialSeed();
	s.GenerateNewSeed();
	// Extremely unlikely to generate the same seed
	// Run a few times to be safe
	bool changed = (s.GetInitialSeed() != oldSeed);
	if (!changed)
	{
		s.GenerateNewSeed();
		changed = (s.GetInitialSeed() != oldSeed);
	}
	EXPECT_TRUE(changed);
}

// =================================================================
// FRandomStream — RandHelper edge cases
// =================================================================

TEST(FRandomStreamTest, RandHelperZeroReturnsZero)
{
	FRandomStream s(1);
	EXPECT_EQ(s.RandHelper(0), 0);
}

TEST(FRandomStreamTest, RandHelperOneReturnsZero)
{
	FRandomStream s(1);
	for (int i = 0; i < 100; ++i)
	{
		EXPECT_EQ(s.RandHelper(1), 0);
	}
}

// =================================================================
// FRandomStream — RandRange bounds
// =================================================================

TEST(FRandomStreamTest, RandRangeStaysInBounds)
{
	FRandomStream s(55);
	for (int i = 0; i < 1000; ++i)
	{
		int32_t v = s.RandRange(10, 20);
		EXPECT_GE(v, 10);
		EXPECT_LE(v, 20);
	}
}

TEST(FRandomStreamTest, FRandRangeStaysInBounds)
{
	FRandomStream s(66);
	for (int i = 0; i < 1000; ++i)
	{
		float v = s.FRandRange(1.0f, 5.0f);
		EXPECT_GE(v, 1.0f);
		EXPECT_LE(v, 5.0f);
	}
}

// =================================================================
// FRandomStream — GetUnitVector
// =================================================================

TEST(FRandomStreamTest, GetUnitVectorHasUnitLength)
{
	FRandomStream s(77);
	for (int i = 0; i < 100; ++i)
	{
		FVector v = s.GetUnitVector();
		float len = std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
		EXPECT_NEAR(len, 1.0f, 1e-4f);
	}
}

// =================================================================
// FRandomStream — RandBool distribution
// =================================================================

TEST(FRandomStreamTest, RandBoolProducesBothValues)
{
	FRandomStream s(88);
	bool sawTrue = false;
	bool sawFalse = false;
	for (int i = 0; i < 1000; ++i)
	{
		if (s.RandBool()) sawTrue = true;
		else sawFalse = true;
		if (sawTrue && sawFalse) break;
	}
	EXPECT_TRUE(sawTrue);
	EXPECT_TRUE(sawFalse);
}

// =================================================================
// FRandomStream — GetFraction range
// =================================================================

TEST(FRandomStreamTest, GetFractionInZeroOneRange)
{
	FRandomStream s(33);
	for (int i = 0; i < 1000; ++i)
	{
		float v = s.GetFraction();
		EXPECT_GE(v, 0.0f);
		EXPECT_LT(v, 1.0f);
	}
}

// =================================================================
// FRandomStream — Copy produces independent stream
// =================================================================

TEST(FRandomStreamTest, CopyProducesIndependentStream)
{
	FRandomStream a(42);
	a.GetFraction(); // advance state
	FRandomStream b(a); // copy

	// Both should produce same values from here
	EXPECT_FLOAT_EQ(a.GetFraction(), b.GetFraction());
	EXPECT_FLOAT_EQ(a.GetFraction(), b.GetFraction());
}

// =================================================================
// FMath — Static random methods
// =================================================================

TEST(FMathRandomTest, FRandInZeroOneRange)
{
	for (int i = 0; i < 1000; ++i)
	{
		float v = FMath::FRand();
		EXPECT_GE(v, 0.0f);
		EXPECT_LT(v, 1.0f);
	}
}

TEST(FMathRandomTest, RandRangeStaysInBounds)
{
	for (int i = 0; i < 1000; ++i)
	{
		int32_t v = FMath::RandRange(5, 15);
		EXPECT_GE(v, 5);
		EXPECT_LE(v, 15);
	}
}

TEST(FMathRandomTest, FRandRangeStaysInBounds)
{
	for (int i = 0; i < 1000; ++i)
	{
		float v = FMath::FRandRange(-1.0f, 1.0f);
		EXPECT_GE(v, -1.0f);
		EXPECT_LE(v, 1.0f);
	}
}

TEST(FMathRandomTest, RandBoolProducesBothValues)
{
	bool sawTrue = false;
	bool sawFalse = false;
	for (int i = 0; i < 1000; ++i)
	{
		if (FMath::RandBool()) sawTrue = true;
		else sawFalse = true;
		if (sawTrue && sawFalse) break;
	}
	EXPECT_TRUE(sawTrue);
	EXPECT_TRUE(sawFalse);
}

TEST(FMathRandomTest, VRandHasUnitLength)
{
	for (int i = 0; i < 100; ++i)
	{
		FVector v = FMath::VRand();
		float len = std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
		EXPECT_NEAR(len, 1.0f, 1e-4f);
	}
}

TEST(FMathRandomTest, RandHelperZeroReturnsZero)
{
	EXPECT_EQ(FMath::RandHelper(0), 0);
}
