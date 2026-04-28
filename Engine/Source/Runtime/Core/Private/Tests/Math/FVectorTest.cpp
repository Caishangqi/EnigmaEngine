// Copyright EnigmaEngine. All Rights Reserved.

/// @file FVectorTest.cpp
/// @brief Unit tests for FVector.

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
#include "Math/Vector.h"

using Enigma::FVector;
using Enigma::FMath;

static constexpr float T = 1e-5f;

// =================================================================
// Constructors
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, DefaultConstructor)
{
	constexpr FVector V;
	TestEqual("EXPECT_EQ", V.X, 0.0f);
	TestEqual("EXPECT_EQ", V.Y, 0.0f);
	TestEqual("EXPECT_EQ", V.Z, 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, ComponentConstructor)
{
	constexpr FVector V(1.0f, 2.0f, 3.0f);
	TestEqual("EXPECT_EQ", V.X, 1.0f);
	TestEqual("EXPECT_EQ", V.Y, 2.0f);
	TestEqual("EXPECT_EQ", V.Z, 3.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, UniformConstructor)
{
	constexpr FVector V(5.0f);
	TestEqual("EXPECT_EQ", V.X, 5.0f);
	TestEqual("EXPECT_EQ", V.Y, 5.0f);
	TestEqual("EXPECT_EQ", V.Z, 5.0f);
}

// =================================================================
// Arithmetic
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, Addition)
{
	constexpr FVector R = FVector(1.0f, 2.0f, 3.0f) + FVector(4.0f, 5.0f, 6.0f);
	TestEqual("EXPECT_EQ", R.X, 5.0f);
	TestEqual("EXPECT_EQ", R.Y, 7.0f);
	TestEqual("EXPECT_EQ", R.Z, 9.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, Subtraction)
{
	constexpr FVector R = FVector(5.0f, 7.0f, 9.0f) - FVector(1.0f, 2.0f, 3.0f);
	TestEqual("EXPECT_EQ", R.X, 4.0f);
	TestEqual("EXPECT_EQ", R.Y, 5.0f);
	TestEqual("EXPECT_EQ", R.Z, 6.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, ScalarMultiplication)
{
	constexpr FVector R = FVector(1.0f, 2.0f, 3.0f) * 2.0f;
	TestEqual("EXPECT_EQ", R.X, 2.0f);
	TestEqual("EXPECT_EQ", R.Y, 4.0f);
	TestEqual("EXPECT_EQ", R.Z, 6.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, ScalarMultiplicationCommutative)
{
	constexpr FVector R = 3.0f * FVector(1.0f, 2.0f, 3.0f);
	TestEqual("EXPECT_EQ", R.X, 3.0f);
	TestEqual("EXPECT_EQ", R.Y, 6.0f);
	TestEqual("EXPECT_EQ", R.Z, 9.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, ScalarDivision)
{
	constexpr FVector R = FVector(6.0f, 8.0f, 10.0f) / 2.0f;
	TestEqual("EXPECT_EQ", R.X, 3.0f);
	TestEqual("EXPECT_EQ", R.Y, 4.0f);
	TestEqual("EXPECT_EQ", R.Z, 5.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, UnaryNegation)
{
	constexpr FVector R = -FVector(1.0f, -2.0f, 3.0f);
	TestEqual("EXPECT_EQ", R.X, -1.0f);
	TestEqual("EXPECT_EQ", R.Y, 2.0f);
	TestEqual("EXPECT_EQ", R.Z, -3.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, CompoundAddition)
{
	FVector V(1.0f, 2.0f, 3.0f);
	V += FVector(4.0f, 5.0f, 6.0f);
	TestEqual("EXPECT_EQ", V.X, 5.0f);
	TestEqual("EXPECT_EQ", V.Y, 7.0f);
	TestEqual("EXPECT_EQ", V.Z, 9.0f);
}

// =================================================================
// Comparison
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, EqualityOperator)
{
	constexpr FVector A(1.0f, 2.0f, 3.0f);
	constexpr FVector B(1.0f, 2.0f, 3.0f);
	constexpr FVector C(1.0f, 2.0f, 4.0f);
	TestTrue("EXPECT_TRUE", A == B);
	TestTrue("EXPECT_TRUE", A != C);
}

// =================================================================
// Size / Normalize
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, SizeKnownValues)
{
	// 3-4-5 triangle extended: sqrt(1+4+4) = 3
	const FVector V(1.0f, 2.0f, 2.0f);
	TestNear("EXPECT_NEAR", V.Size(), 3.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, SizeSquared)
{
	constexpr FVector V(1.0f, 2.0f, 3.0f);
	TestEqual("EXPECT_EQ", V.SizeSquared(), 14.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, GetNormalizedUnit)
{
	const FVector V(0.0f, 3.0f, 0.0f);
	const FVector N = V.GetNormalized();
	TestNear("EXPECT_NEAR", N.X, 0.0f, T);
	TestNear("EXPECT_NEAR", N.Y, 1.0f, T);
	TestNear("EXPECT_NEAR", N.Z, 0.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, GetNormalizedZeroVector)
{
	const FVector V(0.0f, 0.0f, 0.0f);
	const FVector N = V.GetNormalized();
	TestEqual("EXPECT_EQ", N.X, 0.0f);
	TestEqual("EXPECT_EQ", N.Y, 0.0f);
	TestEqual("EXPECT_EQ", N.Z, 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, NormalizeInPlace)
{
	FVector V(0.0f, 0.0f, 5.0f);
	TestTrue("EXPECT_TRUE", V.Normalize());
	TestNear("EXPECT_NEAR", V.Size(), 1.0f, T);
	TestNear("EXPECT_NEAR", V.Z, 1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, NormalizeZeroReturnsFalse)
{
	FVector V(0.0f, 0.0f, 0.0f);
	TestFalse("EXPECT_FALSE", V.Normalize());
	// Should not crash, vector stays zero
	TestEqual("EXPECT_EQ", V.X, 0.0f);
}

// =================================================================
// DotProduct / CrossProduct
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, DotProductOrthogonal)
{
	constexpr float D = FVector::DotProduct(FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f));
	TestEqual("EXPECT_EQ", D, 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, DotProductParallel)
{
	constexpr float D = FVector::DotProduct(FVector(2.0f, 3.0f, 4.0f), FVector(2.0f, 3.0f, 4.0f));
	TestEqual("EXPECT_EQ", D, 29.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, CrossProductXY)
{
	// X cross Y = Z (right-hand rule)
	constexpr FVector C = FVector::CrossProduct(FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f));
	TestEqual("EXPECT_EQ", C.X, 0.0f);
	TestEqual("EXPECT_EQ", C.Y, 0.0f);
	TestEqual("EXPECT_EQ", C.Z, 1.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, CrossProductYZ)
{
	// Y cross Z = X
	constexpr FVector C = FVector::CrossProduct(FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f));
	TestEqual("EXPECT_EQ", C.X, 1.0f);
	TestEqual("EXPECT_EQ", C.Y, 0.0f);
	TestEqual("EXPECT_EQ", C.Z, 0.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, CrossProductAnticommutative)
{
	constexpr FVector A(1.0f, 2.0f, 3.0f);
	constexpr FVector B(4.0f, 5.0f, 6.0f);
	constexpr FVector AB = FVector::CrossProduct(A, B);
	constexpr FVector BA = FVector::CrossProduct(B, A);
	TestEqual("EXPECT_EQ", AB.X, -BA.X);
	TestEqual("EXPECT_EQ", AB.Y, -BA.Y);
	TestEqual("EXPECT_EQ", AB.Z, -BA.Z);
}

// =================================================================
// Distance
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, Distance)
{
	const float D = FVector::Distance(FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 2.0f, 2.0f));
	TestNear("EXPECT_NEAR", D, 3.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, DistSquared)
{
	constexpr float D = FVector::DistSquared(FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 2.0f, 2.0f));
	TestEqual("EXPECT_EQ", D, 9.0f);
}

// =================================================================
// Tolerance comparison
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, EqualsWithinTolerance)
{
	constexpr FVector A(1.0f, 2.0f, 3.0f);
	constexpr FVector B(1.00005f, 2.00005f, 3.00005f);
	TestTrue("EXPECT_TRUE", A.Equals(B));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, IsNearlyZero)
{
	constexpr FVector V(0.00005f, -0.00005f, 0.00001f);
	TestTrue("EXPECT_TRUE", V.IsNearlyZero());
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, IsZero)
{
	TestTrue("EXPECT_TRUE", FVector(0.0f, 0.0f, 0.0f).IsZero());
	TestFalse("EXPECT_FALSE", FVector(0.0f, 0.0f, 1.0f).IsZero());
}

// =================================================================
// Direction constants (Y-up coordinate system)
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, DirectionConstants)
{
	TestEqual("EXPECT_EQ", FVector::UpVector, FVector(0.0f, 1.0f, 0.0f));
	TestEqual("EXPECT_EQ", FVector::DownVector, FVector(0.0f, -1.0f, 0.0f));
	TestEqual("EXPECT_EQ", FVector::ForwardVector, FVector(0.0f, 0.0f, -1.0f));
	TestEqual("EXPECT_EQ", FVector::BackwardVector, FVector(0.0f, 0.0f, 1.0f));
	TestEqual("EXPECT_EQ", FVector::RightVector, FVector(1.0f, 0.0f, 0.0f));
	TestEqual("EXPECT_EQ", FVector::LeftVector, FVector(-1.0f, 0.0f, 0.0f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, ZeroAndOneConstants)
{
	TestEqual("EXPECT_EQ", FVector::ZeroVector, FVector(0.0f, 0.0f, 0.0f));
	TestEqual("EXPECT_EQ", FVector::OneVector, FVector(1.0f, 1.0f, 1.0f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, CoordinateSystemConsistency)
{
	// Right cross Up = -Forward = Backward (right-hand rule)
	// Right(1,0,0) x Up(0,1,0) = (0,0,1) = Backward
	const FVector C = FVector::CrossProduct(FVector::RightVector, FVector::UpVector);
	TestEqual("EXPECT_EQ", C, FVector::BackwardVector);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FVectorTest, DirectionConstantsAreUnit)
{
	TestNear("EXPECT_NEAR", FVector::UpVector.Size(), 1.0f, T);
	TestNear("EXPECT_NEAR", FVector::ForwardVector.Size(), 1.0f, T);
	TestNear("EXPECT_NEAR", FVector::RightVector.Size(), 1.0f, T);
}
