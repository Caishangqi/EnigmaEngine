// Copyright EnigmaEngine. All Rights Reserved.

/// @file FQuatTest.cpp
/// @brief Unit tests for FQuat.

#include <gtest/gtest.h>
#include "Math/Quat.h"
#include "Math/Matrix.h"
#include "Math/Rotator.h"

using Enigma::FQuat;
using Enigma::FVector;
using Enigma::FMatrix;
using Enigma::FRotator;
using Enigma::FMath;

static constexpr float T = 1e-4f;

// =================================================================
// Identity
// =================================================================

TEST(FQuatTest, IdentityComponents)
{
	constexpr FQuat Q;
	EXPECT_EQ(Q.X, 0.0f);
	EXPECT_EQ(Q.Y, 0.0f);
	EXPECT_EQ(Q.Z, 0.0f);
	EXPECT_EQ(Q.W, 1.0f);
}

TEST(FQuatTest, IdentityConstant)
{
	EXPECT_TRUE(FQuat::Identity.Equals(FQuat(0.0f, 0.0f, 0.0f, 1.0f)));
}

TEST(FQuatTest, IdentityRotatesNothing)
{
	const FVector V(1.0f, 2.0f, 3.0f);
	const FVector R = FQuat::Identity.RotateVector(V);
	EXPECT_NEAR(R.X, V.X, T);
	EXPECT_NEAR(R.Y, V.Y, T);
	EXPECT_NEAR(R.Z, V.Z, T);
}

// =================================================================
// Axis-angle construction
// =================================================================

TEST(FQuatTest, AxisAngle90Y)
{
	// 90 degrees around Y: should rotate X-axis to -Z.
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	EXPECT_NEAR(Q.Size(), 1.0f, T);
	const FVector R = Q.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	EXPECT_NEAR(R.X, 0.0f, T);
	EXPECT_NEAR(R.Y, 0.0f, T);
	EXPECT_NEAR(R.Z, -1.0f, T);
}

TEST(FQuatTest, AxisAngle180Z)
{
	// 180 degrees around Z: X -> -X, Y -> -Y.
	const FQuat Q(FVector(0.0f, 0.0f, 1.0f), FMath::DegreesToRadians(180.0f));
	const FVector R = Q.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	EXPECT_NEAR(R.X, -1.0f, T);
	EXPECT_NEAR(R.Y, 0.0f, T);
	EXPECT_NEAR(R.Z, 0.0f, T);
}

TEST(FQuatTest, AxisAngleZeroAngle)
{
	const FQuat Q(FVector(1.0f, 0.0f, 0.0f), 0.0f);
	EXPECT_TRUE(Q.Equals(FQuat::Identity, T));
}

// =================================================================
// Composition
// =================================================================

TEST(FQuatTest, CompositionTwoRotations)
{
	// Two 90-degree rotations around Y = 180 degrees around Y.
	const FQuat Q90(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FQuat Q180 = Q90 * Q90;
	const FVector R = Q180.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	EXPECT_NEAR(R.X, -1.0f, T);
	EXPECT_NEAR(R.Y, 0.0f, T);
	EXPECT_NEAR(R.Z, 0.0f, T);
}

TEST(FQuatTest, CompositionWithInverse)
{
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FQuat R = Q * Q.GetInverse();
	EXPECT_NEAR(R.X, 0.0f, T);
	EXPECT_NEAR(R.Y, 0.0f, T);
	EXPECT_NEAR(R.Z, 0.0f, T);
	EXPECT_NEAR(FMath::Abs(R.W), 1.0f, T);
}

// =================================================================
// RotateVector / UnrotateVector
// =================================================================

TEST(FQuatTest, RotateVector90X)
{
	// 90 degrees around X: Y -> Z, Z -> -Y.
	const FQuat Q(FVector(1.0f, 0.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FVector R = Q.RotateVector(FVector(0.0f, 1.0f, 0.0f));
	EXPECT_NEAR(R.X, 0.0f, T);
	EXPECT_NEAR(R.Y, 0.0f, T);
	EXPECT_NEAR(R.Z, 1.0f, T);
}

TEST(FQuatTest, UnrotateVectorRoundtrip)
{
	const FQuat Q(FVector(1.0f, 1.0f, 1.0f).GetNormalized(), FMath::DegreesToRadians(60.0f));
	const FVector V(3.0f, -5.0f, 7.0f);
	const FVector Rotated = Q.RotateVector(V);
	const FVector Back = Q.UnrotateVector(Rotated);
	EXPECT_NEAR(Back.X, V.X, T);
	EXPECT_NEAR(Back.Y, V.Y, T);
	EXPECT_NEAR(Back.Z, V.Z, T);
}

TEST(FQuatTest, OperatorStarVectorMatchesRotateVector)
{
	const FQuat Q(FVector(0.0f, 0.0f, 1.0f), FMath::DegreesToRadians(45.0f));
	const FVector V(1.0f, 0.0f, 0.0f);
	const FVector A = Q.RotateVector(V);
	const FVector B = Q * V;
	EXPECT_NEAR(A.X, B.X, T);
	EXPECT_NEAR(A.Y, B.Y, T);
	EXPECT_NEAR(A.Z, B.Z, T);
}

// =================================================================
// Slerp
// =================================================================

TEST(FQuatTest, SlerpAlpha0)
{
	const FQuat A = FQuat::Identity;
	const FQuat B(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FQuat R = FQuat::Slerp(A, B, 0.0f);
	EXPECT_TRUE(R.Equals(A, T));
}

TEST(FQuatTest, SlerpAlpha1)
{
	const FQuat A = FQuat::Identity;
	const FQuat B(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FQuat R = FQuat::Slerp(A, B, 1.0f);
	EXPECT_TRUE(R.Equals(B, T));
}

TEST(FQuatTest, SlerpMidpoint)
{
	// Slerp at 0.5 between identity and 90-deg Y should give 45-deg Y.
	const FQuat A = FQuat::Identity;
	const FQuat B(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FQuat Mid = FQuat::Slerp(A, B, 0.5f);
	const FQuat Expected(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	// Compare rotation effect rather than raw components (sign ambiguity).
	const FVector V(1.0f, 0.0f, 0.0f);
	const FVector RM = Mid.RotateVector(V);
	const FVector RE = Expected.RotateVector(V);
	EXPECT_NEAR(RM.X, RE.X, T);
	EXPECT_NEAR(RM.Y, RE.Y, T);
	EXPECT_NEAR(RM.Z, RE.Z, T);
}

TEST(FQuatTest, SlerpNearParallel)
{
	// Nearly identical quaternions should use Nlerp fallback without issues.
	const FQuat A = FQuat::Identity;
	const FQuat B(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(0.01f));
	const FQuat R = FQuat::Slerp(A, B, 0.5f);
	EXPECT_NEAR(R.Size(), 1.0f, T);
}

TEST(FQuatTest, SlerpShortestPath)
{
	// B is negated A rotated 90 deg. Slerp should still take shortest path.
	const FQuat A = FQuat::Identity;
	const FQuat B(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FQuat BNeg(-B.X, -B.Y, -B.Z, -B.W);
	const FQuat R1 = FQuat::Slerp(A, B, 0.5f);
	const FQuat R2 = FQuat::Slerp(A, BNeg, 0.5f);
	// Both should produce the same rotation.
	const FVector V(1.0f, 0.0f, 0.0f);
	const FVector V1 = R1.RotateVector(V);
	const FVector V2 = R2.RotateVector(V);
	EXPECT_NEAR(V1.X, V2.X, T);
	EXPECT_NEAR(V1.Y, V2.Y, T);
	EXPECT_NEAR(V1.Z, V2.Z, T);
}

// =================================================================
// Normalize / Size
// =================================================================

TEST(FQuatTest, SizeUnit)
{
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	EXPECT_NEAR(Q.Size(), 1.0f, T);
}

TEST(FQuatTest, NormalizeNonUnit)
{
	FQuat Q(1.0f, 2.0f, 3.0f, 4.0f);
	EXPECT_TRUE(Q.Normalize());
	EXPECT_NEAR(Q.Size(), 1.0f, T);
}

TEST(FQuatTest, GetNormalizedPreservesDirection)
{
	const FQuat Q(0.0f, 0.0f, 3.0f, 4.0f);
	const FQuat N = Q.GetNormalized();
	EXPECT_NEAR(N.Size(), 1.0f, T);
	EXPECT_NEAR(N.Z / N.W, 3.0f / 4.0f, T);
}

// =================================================================
// GetInverse
// =================================================================

TEST(FQuatTest, InverseUnit)
{
	const FQuat Q(FVector(1.0f, 0.0f, 0.0f), FMath::DegreesToRadians(60.0f));
	const FQuat Inv = Q.GetInverse();
	const FQuat R = Q * Inv;
	EXPECT_NEAR(R.X, 0.0f, T);
	EXPECT_NEAR(R.Y, 0.0f, T);
	EXPECT_NEAR(R.Z, 0.0f, T);
	EXPECT_NEAR(FMath::Abs(R.W), 1.0f, T);
}

// =================================================================
// ToMatrix equivalence
// =================================================================

TEST(FQuatTest, ToMatrixMatchesRotateVector)
{
	const FQuat Q(FVector(1.0f, 1.0f, 0.0f).GetNormalized(), FMath::DegreesToRadians(60.0f));
	const FVector V(1.0f, 0.0f, 0.0f);
	const FVector ByQuat = Q.RotateVector(V);
	const FVector ByMatrix = Q.ToMatrix().TransformVector(V);
	EXPECT_NEAR(ByQuat.X, ByMatrix.X, T);
	EXPECT_NEAR(ByQuat.Y, ByMatrix.Y, T);
	EXPECT_NEAR(ByQuat.Z, ByMatrix.Z, T);
}

TEST(FQuatTest, ToMatrixIdentity)
{
	const FMatrix R = FQuat::Identity.ToMatrix();
	EXPECT_TRUE(R.Equals(FMatrix::Identity, T));
}

// =================================================================
// ToRotator roundtrip
// =================================================================

TEST(FQuatTest, ToRotatorRoundtrip)
{
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FRotator Rot = Q.ToRotator();
	const FQuat Q2 = Rot.Quaternion();
	// Compare rotation effect.
	const FVector V(1.0f, 0.0f, 0.0f);
	const FVector R1 = Q.RotateVector(V);
	const FVector R2 = Q2.RotateVector(V);
	EXPECT_NEAR(R1.X, R2.X, T);
	EXPECT_NEAR(R1.Y, R2.Y, T);
	EXPECT_NEAR(R1.Z, R2.Z, T);
}
