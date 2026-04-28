// Copyright EnigmaEngine. All Rights Reserved.

/// @file RandomStreamTest.cpp
/// @brief Unit tests for FRandomStream and FMath random methods.

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
#include "Math/RandomStream.h"
#include "Math/MathUtility.h"
#include "Math/Vector.h"

#include <cmath>

using Enigma::FRandomStream;
using Enigma::FMath;
using Enigma::FVector;

// =================================================================
// FRandomStream ??Determinism
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, SameSeedProducesSameSequence)
{
	FRandomStream a(42);
	FRandomStream b(42);

	for (int i = 0; i < 100; ++i)
	{
		TestEqual("EXPECT_EQ", a.GetUnsignedInt(), b.GetUnsignedInt());
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, SameSeedProducesSameFractions)
{
	FRandomStream a(123);
	FRandomStream b(123);

	for (int i = 0; i < 100; ++i)
	{
		TestNear("EXPECT_FLOAT_EQ", a.GetFraction(), b.GetFraction(), 1e-6f);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, SameSeedProducesSameRandRange)
{
	FRandomStream a(7);
	FRandomStream b(7);

	for (int i = 0; i < 100; ++i)
	{
		TestEqual("EXPECT_EQ", a.RandRange(0, 1000), b.RandRange(0, 1000));
	}
}
// =================================================================
// FRandomStream ??Reset
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, ResetReproducesSequence)
{
	FRandomStream s(99);

	// Generate first sequence
	float v0 = s.GetFraction();
	float v1 = s.GetFraction();
	float v2 = s.GetFraction();

	// Reset and verify same sequence
	s.Reset();
	TestNear("EXPECT_FLOAT_EQ", s.GetFraction(), v0, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", s.GetFraction(), v1, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", s.GetFraction(), v2, 1e-6f);
}

// =================================================================
// FRandomStream ??GenerateNewSeed
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, GenerateNewSeedChangesSeed)
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
	TestTrue("EXPECT_TRUE", changed);
}

// =================================================================
// FRandomStream ??RandHelper edge cases
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, RandHelperZeroReturnsZero)
{
	FRandomStream s(1);
	TestEqual("EXPECT_EQ", s.RandHelper(0), 0);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, RandHelperOneReturnsZero)
{
	FRandomStream s(1);
	for (int i = 0; i < 100; ++i)
	{
		TestEqual("EXPECT_EQ", s.RandHelper(1), 0);
	}
}

// =================================================================
// FRandomStream ??RandRange bounds
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, RandRangeStaysInBounds)
{
	FRandomStream s(55);
	for (int i = 0; i < 1000; ++i)
	{
		int32_t v = s.RandRange(10, 20);
		TestGreaterThanOrEqual("EXPECT_GE", v, 10);
		TestLessThanOrEqual("EXPECT_LE", v, 20);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, FRandRangeStaysInBounds)
{
	FRandomStream s(66);
	for (int i = 0; i < 1000; ++i)
	{
		float v = s.FRandRange(1.0f, 5.0f);
		TestGreaterThanOrEqual("EXPECT_GE", v, 1.0f);
		TestLessThanOrEqual("EXPECT_LE", v, 5.0f);
	}
}

// =================================================================
// FRandomStream ??GetUnitVector
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, GetUnitVectorHasUnitLength)
{
	FRandomStream s(77);
	for (int i = 0; i < 100; ++i)
	{
		FVector v = s.GetUnitVector();
		float len = std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
		TestNear("EXPECT_NEAR", len, 1.0f, 1e-4f);
	}
}

// =================================================================
// FRandomStream ??RandBool distribution
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, RandBoolProducesBothValues)
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
	TestTrue("EXPECT_TRUE", sawTrue);
	TestTrue("EXPECT_TRUE", sawFalse);
}

// =================================================================
// FRandomStream ??GetFraction range
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, GetFractionInZeroOneRange)
{
	FRandomStream s(33);
	for (int i = 0; i < 1000; ++i)
	{
		float v = s.GetFraction();
		TestGreaterThanOrEqual("EXPECT_GE", v, 0.0f);
		TestLessThan("EXPECT_LT", v, 1.0f);
	}
}

// =================================================================
// FRandomStream ??Copy produces independent stream
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FRandomStreamTest, CopyProducesIndependentStream)
{
	FRandomStream a(42);
	a.GetFraction(); // advance state
	FRandomStream b(a); // copy

	// Both should produce same values from here
	TestNear("EXPECT_FLOAT_EQ", a.GetFraction(), b.GetFraction(), 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", a.GetFraction(), b.GetFraction(), 1e-6f);
}

// =================================================================
// FMath ??Static random methods
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathRandomTest, FRandInZeroOneRange)
{
	for (int i = 0; i < 1000; ++i)
	{
		float v = FMath::FRand();
		TestGreaterThanOrEqual("EXPECT_GE", v, 0.0f);
		TestLessThan("EXPECT_LT", v, 1.0f);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathRandomTest, RandRangeStaysInBounds)
{
	for (int i = 0; i < 1000; ++i)
	{
		int32_t v = FMath::RandRange(5, 15);
		TestGreaterThanOrEqual("EXPECT_GE", v, 5);
		TestLessThanOrEqual("EXPECT_LE", v, 15);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathRandomTest, FRandRangeStaysInBounds)
{
	for (int i = 0; i < 1000; ++i)
	{
		float v = FMath::FRandRange(-1.0f, 1.0f);
		TestGreaterThanOrEqual("EXPECT_GE", v, -1.0f);
		TestLessThanOrEqual("EXPECT_LE", v, 1.0f);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathRandomTest, RandBoolProducesBothValues)
{
	bool sawTrue = false;
	bool sawFalse = false;
	for (int i = 0; i < 1000; ++i)
	{
		if (FMath::RandBool()) sawTrue = true;
		else sawFalse = true;
		if (sawTrue && sawFalse) break;
	}
	TestTrue("EXPECT_TRUE", sawTrue);
	TestTrue("EXPECT_TRUE", sawFalse);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathRandomTest, VRandHasUnitLength)
{
	for (int i = 0; i < 100; ++i)
	{
		FVector v = FMath::VRand();
		float len = std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
		TestNear("EXPECT_NEAR", len, 1.0f, 1e-4f);
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMathRandomTest, RandHelperZeroReturnsZero)
{
	TestEqual("EXPECT_EQ", FMath::RandHelper(0), 0);
}
