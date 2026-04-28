// Copyright EnigmaEngine. All Rights Reserved.

/// @file FMatrixTest.cpp
/// @brief Unit tests for FMatrix.

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
#include "Math/Matrix.h"
#include "Math/Quat.h"

using Enigma::FMatrix;
using Enigma::FVector;
using Enigma::FQuat;
using Enigma::FMath;

static constexpr float T = 1e-4f;

// =================================================================
// Identity
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, IdentityDiagonal)
{
	const FMatrix& I = FMatrix::Identity;
	for (int R = 0; R < 4; ++R)
	{
		for (int C = 0; C < 4; ++C)
		{
			TestEqual("EXPECT_EQ", I.M[R][C], (R == C) ? 1.0f : 0.0f);
		}
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, IdentityMultiply)
{
	const FMatrix A(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	const FMatrix R = A * FMatrix::Identity;
	TestTrue("EXPECT_TRUE", R.Equals(A));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, IdentityMultiplyLeft)
{
	const FMatrix A(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	const FMatrix R = FMatrix::Identity * A;
	TestTrue("EXPECT_TRUE", R.Equals(A));
}

// =================================================================
// Multiply
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, MultiplyKnownValues)
{
	// Simple 2x scale * translation should put scaled translation in Row 3.
	const FMatrix S = FMatrix::MakeScale(2.0f);
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(1.0f, 2.0f, 3.0f));
	const FMatrix R = S * Tr;
	// Row-vector: apply S first, then Tr. Scale doesn't affect translation row of Tr.
	// Result should be scale 2 with translation (1,2,3).
	TestNear("EXPECT_NEAR", R.M[0][0], 2.0f, T);
	TestNear("EXPECT_NEAR", R.M[1][1], 2.0f, T);
	TestNear("EXPECT_NEAR", R.M[2][2], 2.0f, T);
	TestNear("EXPECT_NEAR", R.M[3][0], 1.0f, T);
	TestNear("EXPECT_NEAR", R.M[3][1], 2.0f, T);
	TestNear("EXPECT_NEAR", R.M[3][2], 3.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, MultiplyNonCommutative)
{
	const FMatrix A = FMatrix::MakeTranslation(FVector(1.0f, 0.0f, 0.0f));
	const FMatrix B = FMatrix::MakeScale(FVector(2.0f, 1.0f, 1.0f));
	const FMatrix AB = A * B;
	const FMatrix BA = B * A;
	// A*B != B*A for translation * non-uniform scale
	TestFalse("EXPECT_FALSE", AB.Equals(BA));
}

// =================================================================
// Transform operations
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, TransformPositionTranslation)
{
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(10.0f, 20.0f, 30.0f));
	const FVector R = Tr.TransformPosition(FVector(1.0f, 2.0f, 3.0f));
	TestNear("EXPECT_NEAR", R.X, 11.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 22.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 33.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, TransformPositionScale)
{
	const FMatrix S = FMatrix::MakeScale(FVector(2.0f, 3.0f, 4.0f));
	const FVector R = S.TransformPosition(FVector(1.0f, 1.0f, 1.0f));
	TestNear("EXPECT_NEAR", R.X, 2.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 3.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 4.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, TransformVectorIgnoresTranslation)
{
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(100.0f, 200.0f, 300.0f));
	const FVector R = Tr.TransformVector(FVector(1.0f, 2.0f, 3.0f));
	TestNear("EXPECT_NEAR", R.X, 1.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 2.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 3.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, TransformPositionRotation90Y)
{
	// 90 degrees around Y axis: X -> -Z, Z -> X
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FMatrix R = FMatrix::MakeRotation(Q);
	const FVector V = R.TransformPosition(FVector(1.0f, 0.0f, 0.0f));
	TestNear("EXPECT_NEAR", V.X, 0.0f, T);
	TestNear("EXPECT_NEAR", V.Y, 0.0f, T);
	TestNear("EXPECT_NEAR", V.Z, -1.0f, T);
}

// =================================================================
// Transpose
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, TransposeSymmetric)
{
	// Identity is symmetric, transpose should equal itself.
	const FMatrix R = FMatrix::Identity.GetTransposed();
	TestTrue("EXPECT_TRUE", R.Equals(FMatrix::Identity));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, TransposeSwapsRowCol)
{
	const FMatrix A(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	const FMatrix AT = A.GetTransposed();
	for (int R = 0; R < 4; ++R)
	{
		for (int C = 0; C < 4; ++C)
		{
			TestEqual("EXPECT_EQ", AT.M[R][C], A.M[C][R]);
		}
	}
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, DoubleTransposeIsOriginal)
{
	const FMatrix A(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	const FMatrix R = A.GetTransposed().GetTransposed();
	TestTrue("EXPECT_TRUE", R.Equals(A));
}

// =================================================================
// Determinant
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, DeterminantIdentity)
{
	TestNear("EXPECT_NEAR", FMatrix::Identity.Determinant(), 1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, DeterminantScale)
{
	const FMatrix S = FMatrix::MakeScale(FVector(2.0f, 3.0f, 4.0f));
	TestNear("EXPECT_NEAR", S.Determinant(), 24.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, DeterminantSingular)
{
	// All-zero matrix has determinant 0.
	const FMatrix Z(
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f);
	TestNear("EXPECT_NEAR", Z.Determinant(), 0.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, DeterminantRotation)
{
	// Pure rotation has determinant 1.
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FMatrix R = FMatrix::MakeRotation(Q);
	TestNear("EXPECT_NEAR", R.Determinant(), 1.0f, T);
}

// =================================================================
// Inverse
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, InverseTimesOriginalIsIdentity)
{
	const FMatrix A = FMatrix::MakeTranslation(FVector(3.0f, -7.0f, 11.0f));
	const FMatrix Inv = A.GetInverse();
	const FMatrix R = A * Inv;
	TestTrue("EXPECT_TRUE", R.Equals(FMatrix::Identity, 1e-3f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, InverseRotation)
{
	const FQuat Q(FVector(1.0f, 0.0f, 0.0f), FMath::DegreesToRadians(60.0f));
	const FMatrix Rot = FMatrix::MakeRotation(Q);
	const FMatrix R = Rot * Rot.GetInverse();
	TestTrue("EXPECT_TRUE", R.Equals(FMatrix::Identity, 1e-3f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, InverseScale)
{
	const FMatrix S = FMatrix::MakeScale(FVector(2.0f, 4.0f, 0.5f));
	const FMatrix R = S * S.GetInverse();
	TestTrue("EXPECT_TRUE", R.Equals(FMatrix::Identity, 1e-3f));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, InverseSingularReturnsIdentity)
{
	const FMatrix Z(
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f);
	const FMatrix Inv = Z.GetInverse();
	TestTrue("EXPECT_TRUE", Inv.Equals(FMatrix::Identity));
}

// =================================================================
// Factory methods
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, MakeTranslationRow3)
{
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(5.0f, 6.0f, 7.0f));
	TestNear("EXPECT_NEAR", Tr.M[3][0], 5.0f, T);
	TestNear("EXPECT_NEAR", Tr.M[3][1], 6.0f, T);
	TestNear("EXPECT_NEAR", Tr.M[3][2], 7.0f, T);
	TestNear("EXPECT_NEAR", Tr.M[3][3], 1.0f, T);
	// Upper 3x3 should be identity.
	TestNear("EXPECT_NEAR", Tr.M[0][0], 1.0f, T);
	TestNear("EXPECT_NEAR", Tr.M[1][1], 1.0f, T);
	TestNear("EXPECT_NEAR", Tr.M[2][2], 1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, MakeScaleUniform)
{
	const FMatrix S = FMatrix::MakeScale(3.0f);
	TestNear("EXPECT_NEAR", S.M[0][0], 3.0f, T);
	TestNear("EXPECT_NEAR", S.M[1][1], 3.0f, T);
	TestNear("EXPECT_NEAR", S.M[2][2], 3.0f, T);
	TestNear("EXPECT_NEAR", S.M[3][3], 1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, MakeScaleNonUniform)
{
	const FMatrix S = FMatrix::MakeScale(FVector(1.0f, 2.0f, 3.0f));
	TestNear("EXPECT_NEAR", S.M[0][0], 1.0f, T);
	TestNear("EXPECT_NEAR", S.M[1][1], 2.0f, T);
	TestNear("EXPECT_NEAR", S.M[2][2], 3.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, MakeRotationIdentityQuat)
{
	const FMatrix R = FMatrix::MakeRotation(FQuat::Identity);
	TestTrue("EXPECT_TRUE", R.Equals(FMatrix::Identity));
}

// =================================================================
// Equals
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, EqualsWithinTolerance)
{
	const FMatrix A = FMatrix::Identity;
	FMatrix B = FMatrix::Identity;
	B.M[0][0] = 1.0f + 1e-5f;
	TestTrue("EXPECT_TRUE", A.Equals(B));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FMatrixTest, EqualsOutsideTolerance)
{
	const FMatrix A = FMatrix::Identity;
	FMatrix B = FMatrix::Identity;
	B.M[0][0] = 2.0f;
	TestFalse("EXPECT_FALSE", A.Equals(B));
}
