// Copyright EnigmaEngine. All Rights Reserved.

/// @file FMatrixTest.cpp
/// @brief Unit tests for FMatrix.

#include <gtest/gtest.h>
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

TEST(FMatrixTest, IdentityDiagonal)
{
	const FMatrix& I = FMatrix::Identity;
	for (int R = 0; R < 4; ++R)
	{
		for (int C = 0; C < 4; ++C)
		{
			EXPECT_EQ(I.M[R][C], (R == C) ? 1.0f : 0.0f);
		}
	}
}

TEST(FMatrixTest, IdentityMultiply)
{
	const FMatrix A(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	const FMatrix R = A * FMatrix::Identity;
	EXPECT_TRUE(R.Equals(A));
}

TEST(FMatrixTest, IdentityMultiplyLeft)
{
	const FMatrix A(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	const FMatrix R = FMatrix::Identity * A;
	EXPECT_TRUE(R.Equals(A));
}

// =================================================================
// Multiply
// =================================================================

TEST(FMatrixTest, MultiplyKnownValues)
{
	// Simple 2x scale * translation should put scaled translation in Row 3.
	const FMatrix S = FMatrix::MakeScale(2.0f);
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(1.0f, 2.0f, 3.0f));
	const FMatrix R = S * Tr;
	// Row-vector: apply S first, then Tr. Scale doesn't affect translation row of Tr.
	// Result should be scale 2 with translation (1,2,3).
	EXPECT_NEAR(R.M[0][0], 2.0f, T);
	EXPECT_NEAR(R.M[1][1], 2.0f, T);
	EXPECT_NEAR(R.M[2][2], 2.0f, T);
	EXPECT_NEAR(R.M[3][0], 1.0f, T);
	EXPECT_NEAR(R.M[3][1], 2.0f, T);
	EXPECT_NEAR(R.M[3][2], 3.0f, T);
}

TEST(FMatrixTest, MultiplyNonCommutative)
{
	const FMatrix A = FMatrix::MakeTranslation(FVector(1.0f, 0.0f, 0.0f));
	const FMatrix B = FMatrix::MakeScale(FVector(2.0f, 1.0f, 1.0f));
	const FMatrix AB = A * B;
	const FMatrix BA = B * A;
	// A*B != B*A for translation * non-uniform scale
	EXPECT_FALSE(AB.Equals(BA));
}

// =================================================================
// Transform operations
// =================================================================

TEST(FMatrixTest, TransformPositionTranslation)
{
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(10.0f, 20.0f, 30.0f));
	const FVector R = Tr.TransformPosition(FVector(1.0f, 2.0f, 3.0f));
	EXPECT_NEAR(R.X, 11.0f, T);
	EXPECT_NEAR(R.Y, 22.0f, T);
	EXPECT_NEAR(R.Z, 33.0f, T);
}

TEST(FMatrixTest, TransformPositionScale)
{
	const FMatrix S = FMatrix::MakeScale(FVector(2.0f, 3.0f, 4.0f));
	const FVector R = S.TransformPosition(FVector(1.0f, 1.0f, 1.0f));
	EXPECT_NEAR(R.X, 2.0f, T);
	EXPECT_NEAR(R.Y, 3.0f, T);
	EXPECT_NEAR(R.Z, 4.0f, T);
}

TEST(FMatrixTest, TransformVectorIgnoresTranslation)
{
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(100.0f, 200.0f, 300.0f));
	const FVector R = Tr.TransformVector(FVector(1.0f, 2.0f, 3.0f));
	EXPECT_NEAR(R.X, 1.0f, T);
	EXPECT_NEAR(R.Y, 2.0f, T);
	EXPECT_NEAR(R.Z, 3.0f, T);
}

TEST(FMatrixTest, TransformPositionRotation90Y)
{
	// 90 degrees around Y axis: X -> -Z, Z -> X
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FMatrix R = FMatrix::MakeRotation(Q);
	const FVector V = R.TransformPosition(FVector(1.0f, 0.0f, 0.0f));
	EXPECT_NEAR(V.X, 0.0f, T);
	EXPECT_NEAR(V.Y, 0.0f, T);
	EXPECT_NEAR(V.Z, -1.0f, T);
}

// =================================================================
// Transpose
// =================================================================

TEST(FMatrixTest, TransposeSymmetric)
{
	// Identity is symmetric, transpose should equal itself.
	const FMatrix R = FMatrix::Identity.GetTransposed();
	EXPECT_TRUE(R.Equals(FMatrix::Identity));
}

TEST(FMatrixTest, TransposeSwapsRowCol)
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
			EXPECT_EQ(AT.M[R][C], A.M[C][R]);
		}
	}
}

TEST(FMatrixTest, DoubleTransposeIsOriginal)
{
	const FMatrix A(
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f);
	const FMatrix R = A.GetTransposed().GetTransposed();
	EXPECT_TRUE(R.Equals(A));
}

// =================================================================
// Determinant
// =================================================================

TEST(FMatrixTest, DeterminantIdentity)
{
	EXPECT_NEAR(FMatrix::Identity.Determinant(), 1.0f, T);
}

TEST(FMatrixTest, DeterminantScale)
{
	const FMatrix S = FMatrix::MakeScale(FVector(2.0f, 3.0f, 4.0f));
	EXPECT_NEAR(S.Determinant(), 24.0f, T);
}

TEST(FMatrixTest, DeterminantSingular)
{
	// All-zero matrix has determinant 0.
	const FMatrix Z(
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f);
	EXPECT_NEAR(Z.Determinant(), 0.0f, T);
}

TEST(FMatrixTest, DeterminantRotation)
{
	// Pure rotation has determinant 1.
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FMatrix R = FMatrix::MakeRotation(Q);
	EXPECT_NEAR(R.Determinant(), 1.0f, T);
}

// =================================================================
// Inverse
// =================================================================

TEST(FMatrixTest, InverseTimesOriginalIsIdentity)
{
	const FMatrix A = FMatrix::MakeTranslation(FVector(3.0f, -7.0f, 11.0f));
	const FMatrix Inv = A.GetInverse();
	const FMatrix R = A * Inv;
	EXPECT_TRUE(R.Equals(FMatrix::Identity, 1e-3f));
}

TEST(FMatrixTest, InverseRotation)
{
	const FQuat Q(FVector(1.0f, 0.0f, 0.0f), FMath::DegreesToRadians(60.0f));
	const FMatrix Rot = FMatrix::MakeRotation(Q);
	const FMatrix R = Rot * Rot.GetInverse();
	EXPECT_TRUE(R.Equals(FMatrix::Identity, 1e-3f));
}

TEST(FMatrixTest, InverseScale)
{
	const FMatrix S = FMatrix::MakeScale(FVector(2.0f, 4.0f, 0.5f));
	const FMatrix R = S * S.GetInverse();
	EXPECT_TRUE(R.Equals(FMatrix::Identity, 1e-3f));
}

TEST(FMatrixTest, InverseSingularReturnsIdentity)
{
	const FMatrix Z(
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f);
	const FMatrix Inv = Z.GetInverse();
	EXPECT_TRUE(Inv.Equals(FMatrix::Identity));
}

// =================================================================
// Factory methods
// =================================================================

TEST(FMatrixTest, MakeTranslationRow3)
{
	const FMatrix Tr = FMatrix::MakeTranslation(FVector(5.0f, 6.0f, 7.0f));
	EXPECT_NEAR(Tr.M[3][0], 5.0f, T);
	EXPECT_NEAR(Tr.M[3][1], 6.0f, T);
	EXPECT_NEAR(Tr.M[3][2], 7.0f, T);
	EXPECT_NEAR(Tr.M[3][3], 1.0f, T);
	// Upper 3x3 should be identity.
	EXPECT_NEAR(Tr.M[0][0], 1.0f, T);
	EXPECT_NEAR(Tr.M[1][1], 1.0f, T);
	EXPECT_NEAR(Tr.M[2][2], 1.0f, T);
}

TEST(FMatrixTest, MakeScaleUniform)
{
	const FMatrix S = FMatrix::MakeScale(3.0f);
	EXPECT_NEAR(S.M[0][0], 3.0f, T);
	EXPECT_NEAR(S.M[1][1], 3.0f, T);
	EXPECT_NEAR(S.M[2][2], 3.0f, T);
	EXPECT_NEAR(S.M[3][3], 1.0f, T);
}

TEST(FMatrixTest, MakeScaleNonUniform)
{
	const FMatrix S = FMatrix::MakeScale(FVector(1.0f, 2.0f, 3.0f));
	EXPECT_NEAR(S.M[0][0], 1.0f, T);
	EXPECT_NEAR(S.M[1][1], 2.0f, T);
	EXPECT_NEAR(S.M[2][2], 3.0f, T);
}

TEST(FMatrixTest, MakeRotationIdentityQuat)
{
	const FMatrix R = FMatrix::MakeRotation(FQuat::Identity);
	EXPECT_TRUE(R.Equals(FMatrix::Identity));
}

// =================================================================
// Equals
// =================================================================

TEST(FMatrixTest, EqualsWithinTolerance)
{
	const FMatrix A = FMatrix::Identity;
	FMatrix B = FMatrix::Identity;
	B.M[0][0] = 1.0f + 1e-5f;
	EXPECT_TRUE(A.Equals(B));
}

TEST(FMatrixTest, EqualsOutsideTolerance)
{
	const FMatrix A = FMatrix::Identity;
	FMatrix B = FMatrix::Identity;
	B.M[0][0] = 2.0f;
	EXPECT_FALSE(A.Equals(B));
}
