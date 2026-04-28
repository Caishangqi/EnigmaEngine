// Copyright EnigmaEngine. All Rights Reserved.

/// @file FIntVectorTest.cpp
/// @brief Unit tests for FIntVector.

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
#include "Math/IntVector.h"
#include "Math/Vector.h"

using Enigma::FIntVector;
using Enigma::FVector;

// =================================================================
// Constructors
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, DefaultConstructor)
{
	constexpr FIntVector V;
	TestEqual("EXPECT_EQ", V.X, 0);
	TestEqual("EXPECT_EQ", V.Y, 0);
	TestEqual("EXPECT_EQ", V.Z, 0);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ComponentConstructor)
{
	constexpr FIntVector V(1, 2, 3);
	TestEqual("EXPECT_EQ", V.X, 1);
	TestEqual("EXPECT_EQ", V.Y, 2);
	TestEqual("EXPECT_EQ", V.Z, 3);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, UniformConstructor)
{
	constexpr FIntVector V(7);
	TestEqual("EXPECT_EQ", V.X, 7);
	TestEqual("EXPECT_EQ", V.Y, 7);
	TestEqual("EXPECT_EQ", V.Z, 7);
}

// =================================================================
// Arithmetic
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, Addition)
{
	constexpr FIntVector R = FIntVector(1, 2, 3) + FIntVector(4, 5, 6);
	TestEqual("EXPECT_EQ", R.X, 5);
	TestEqual("EXPECT_EQ", R.Y, 7);
	TestEqual("EXPECT_EQ", R.Z, 9);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, Subtraction)
{
	constexpr FIntVector R = FIntVector(5, 7, 9) - FIntVector(1, 2, 3);
	TestEqual("EXPECT_EQ", R.X, 4);
	TestEqual("EXPECT_EQ", R.Y, 5);
	TestEqual("EXPECT_EQ", R.Z, 6);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ComponentMultiplication)
{
	constexpr FIntVector R = FIntVector(2, 3, 4) * FIntVector(5, 6, 7);
	TestEqual("EXPECT_EQ", R.X, 10);
	TestEqual("EXPECT_EQ", R.Y, 18);
	TestEqual("EXPECT_EQ", R.Z, 28);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ComponentDivision)
{
	constexpr FIntVector R = FIntVector(10, 20, 30) / FIntVector(2, 5, 10);
	TestEqual("EXPECT_EQ", R.X, 5);
	TestEqual("EXPECT_EQ", R.Y, 4);
	TestEqual("EXPECT_EQ", R.Z, 3);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, Modulo)
{
	constexpr FIntVector R = FIntVector(10, 7, 15) % FIntVector(3, 4, 6);
	TestEqual("EXPECT_EQ", R.X, 1);
	TestEqual("EXPECT_EQ", R.Y, 3);
	TestEqual("EXPECT_EQ", R.Z, 3);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ModuloNegative)
{
	// C++ truncation semantics: -10 % 3 = -1
	constexpr FIntVector R = FIntVector(-10, -7, 15) % FIntVector(3, 4, 6);
	TestEqual("EXPECT_EQ", R.X, -1);
	TestEqual("EXPECT_EQ", R.Y, -3);
	TestEqual("EXPECT_EQ", R.Z, 3);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ScalarMultiplication)
{
	constexpr FIntVector R = FIntVector(1, 2, 3) * 3;
	TestEqual("EXPECT_EQ", R.X, 3);
	TestEqual("EXPECT_EQ", R.Y, 6);
	TestEqual("EXPECT_EQ", R.Z, 9);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ScalarDivision)
{
	constexpr FIntVector R = FIntVector(9, 6, 3) / 3;
	TestEqual("EXPECT_EQ", R.X, 3);
	TestEqual("EXPECT_EQ", R.Y, 2);
	TestEqual("EXPECT_EQ", R.Z, 1);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, UnaryNegation)
{
	constexpr FIntVector R = -FIntVector(1, -2, 3);
	TestEqual("EXPECT_EQ", R.X, -1);
	TestEqual("EXPECT_EQ", R.Y, 2);
	TestEqual("EXPECT_EQ", R.Z, -3);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, CompoundAddition)
{
	FIntVector V(1, 2, 3);
	V += FIntVector(4, 5, 6);
	TestEqual("EXPECT_EQ", V.X, 5);
	TestEqual("EXPECT_EQ", V.Y, 7);
	TestEqual("EXPECT_EQ", V.Z, 9);
}

// =================================================================
// Comparison
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, EqualityOperator)
{
	constexpr FIntVector A(1, 2, 3);
	constexpr FIntVector B(1, 2, 3);
	constexpr FIntVector C(1, 2, 4);
	TestTrue("EXPECT_TRUE", A == B);
	TestTrue("EXPECT_TRUE", A != C);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, IsZero)
{
	TestTrue("EXPECT_TRUE", FIntVector(0, 0, 0).IsZero());
	TestFalse("EXPECT_FALSE", FIntVector(1, 0, 0).IsZero());
}

// =================================================================
// Direction constants (Y-up coordinate system)
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, DirectionConstants)
{
	TestEqual("EXPECT_EQ", FIntVector::Up(), FIntVector(0, 1, 0));
	TestEqual("EXPECT_EQ", FIntVector::Down(), FIntVector(0, -1, 0));
	TestEqual("EXPECT_EQ", FIntVector::Forward(), FIntVector(0, 0, -1));
	TestEqual("EXPECT_EQ", FIntVector::Backward(), FIntVector(0, 0, 1));
	TestEqual("EXPECT_EQ", FIntVector::Right(), FIntVector(1, 0, 0));
	TestEqual("EXPECT_EQ", FIntVector::Left(), FIntVector(-1, 0, 0));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, StaticConstants)
{
	TestEqual("EXPECT_EQ", FIntVector::ZeroValue, FIntVector(0, 0, 0));
	TestEqual("EXPECT_EQ", FIntVector::OneValue, FIntVector(1, 1, 1));
}

// =================================================================
// Float conversion
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ToFloat)
{
	const FIntVector IV(3, -4, 5);
	const FVector FV = IV.ToFloat();
	TestEqual("EXPECT_EQ", FV.X, 3.0f);
	TestEqual("EXPECT_EQ", FV.Y, -4.0f);
	TestEqual("EXPECT_EQ", FV.Z, 5.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, FromFloatPositive)
{
	const FIntVector IV = FIntVector::FromFloat(FVector(1.7f, 2.3f, 3.9f));
	TestEqual("EXPECT_EQ", IV.X, 1);
	TestEqual("EXPECT_EQ", IV.Y, 2);
	TestEqual("EXPECT_EQ", IV.Z, 3);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, FromFloatNegativeFloor)
{
	// floor(-1.3) = -2, floor(-2.7) = -3, floor(-0.1) = -1
	const FIntVector IV = FIntVector::FromFloat(FVector(-1.3f, -2.7f, -0.1f));
	TestEqual("EXPECT_EQ", IV.X, -2);
	TestEqual("EXPECT_EQ", IV.Y, -3);
	TestEqual("EXPECT_EQ", IV.Z, -1);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, FromFloatExactIntegers)
{
	const FIntVector IV = FIntVector::FromFloat(FVector(5.0f, -3.0f, 0.0f));
	TestEqual("EXPECT_EQ", IV.X, 5);
	TestEqual("EXPECT_EQ", IV.Y, -3);
	TestEqual("EXPECT_EQ", IV.Z, 0);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, RoundtripConversion)
{
	const FIntVector Original(10, -20, 30);
	const FIntVector Roundtrip = FIntVector::FromFloat(Original.ToFloat());
	TestEqual("EXPECT_EQ", Original, Roundtrip);
}

// =================================================================
// Edge cases
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, LargeValues)
{
	constexpr FIntVector V(2147483647, -2147483647, 0);  // INT32_MAX
	constexpr FIntVector R = V + FIntVector(0, 0, 1);
	TestEqual("EXPECT_EQ", R.Z, 1);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FIntVectorTest, ConstexprArithmetic)
{
	static_assert(FIntVector(1, 2, 3) + FIntVector(4, 5, 6) == FIntVector(5, 7, 9),
		"FIntVector arithmetic must be constexpr");
}
