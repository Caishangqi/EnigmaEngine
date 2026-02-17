// Copyright EnigmaEngine. All Rights Reserved.

/// @file FRotatorTest.cpp
/// @brief Unit tests for FRotator.

#include <gtest/gtest.h>
#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "Math/Matrix.h"

using Enigma::FRotator;
using Enigma::FQuat;
using Enigma::FVector;
using Enigma::FMatrix;
using Enigma::FMath;

static constexpr float T = 1e-4f;

// =================================================================
// Constructors
// =================================================================

TEST(FRotatorTest, DefaultConstructor)
{
	constexpr FRotator R;
	EXPECT_EQ(R.Pitch, 0.0f);
	EXPECT_EQ(R.Yaw, 0.0f);
	EXPECT_EQ(R.Roll, 0.0f);
}

TEST(FRotatorTest, ComponentConstructor)
{
	constexpr FRotator R(10.0f, 20.0f, 30.0f);
	EXPECT_EQ(R.Pitch, 10.0f);
	EXPECT_EQ(R.Yaw, 20.0f);
	EXPECT_EQ(R.Roll, 30.0f);
}

TEST(FRotatorTest, ConstructFromQuat)
{
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FRotator R(Q);
	EXPECT_NEAR(R.Yaw, 45.0f, T);
	EXPECT_NEAR(R.Pitch, 0.0f, T);
	EXPECT_NEAR(R.Roll, 0.0f, T);
}

// =================================================================
// Arithmetic
// =================================================================

TEST(FRotatorTest, Addition)
{
	constexpr FRotator A(10.0f, 20.0f, 30.0f);
	constexpr FRotator B(5.0f, 10.0f, 15.0f);
	constexpr FRotator R = A + B;
	EXPECT_EQ(R.Pitch, 15.0f);
	EXPECT_EQ(R.Yaw, 30.0f);
	EXPECT_EQ(R.Roll, 45.0f);
}

TEST(FRotatorTest, Subtraction)
{
	constexpr FRotator A(10.0f, 20.0f, 30.0f);
	constexpr FRotator B(5.0f, 10.0f, 15.0f);
	constexpr FRotator R = A - B;
	EXPECT_EQ(R.Pitch, 5.0f);
	EXPECT_EQ(R.Yaw, 10.0f);
	EXPECT_EQ(R.Roll, 15.0f);
}

TEST(FRotatorTest, ScalarMultiply)
{
	constexpr FRotator A(10.0f, 20.0f, 30.0f);
	constexpr FRotator R = A * 2.0f;
	EXPECT_EQ(R.Pitch, 20.0f);
	EXPECT_EQ(R.Yaw, 40.0f);
	EXPECT_EQ(R.Roll, 60.0f);
}

TEST(FRotatorTest, UnaryNegation)
{
	constexpr FRotator A(10.0f, -20.0f, 30.0f);
	constexpr FRotator R = -A;
	EXPECT_EQ(R.Pitch, -10.0f);
	EXPECT_EQ(R.Yaw, 20.0f);
	EXPECT_EQ(R.Roll, -30.0f);
}

TEST(FRotatorTest, CompoundAddition)
{
	FRotator R(10.0f, 20.0f, 30.0f);
	R += FRotator(5.0f, 5.0f, 5.0f);
	EXPECT_EQ(R.Pitch, 15.0f);
	EXPECT_EQ(R.Yaw, 25.0f);
	EXPECT_EQ(R.Roll, 35.0f);
}

// =================================================================
// Comparison
// =================================================================

TEST(FRotatorTest, EqualityOperator)
{
	constexpr FRotator A(10.0f, 20.0f, 30.0f);
	constexpr FRotator B(10.0f, 20.0f, 30.0f);
	EXPECT_TRUE(A == B);
	EXPECT_FALSE(A != B);
}

TEST(FRotatorTest, EqualsWithTolerance)
{
	constexpr FRotator A(10.0f, 20.0f, 30.0f);
	constexpr FRotator B(10.00005f, 20.00005f, 30.00005f);
	EXPECT_TRUE(A.Equals(B));
}

// =================================================================
// Normalization
// =================================================================

TEST(FRotatorTest, NormalizePositive)
{
	const FRotator R(200.0f, 0.0f, 0.0f);
	const FRotator N = R.GetNormalized();
	// 200 -> -160 (normalized to [-180, 180))
	EXPECT_NEAR(N.Pitch, -160.0f, T);
}

TEST(FRotatorTest, NormalizeNegative)
{
	const FRotator R(-200.0f, 0.0f, 0.0f);
	const FRotator N = R.GetNormalized();
	// -200 -> 160
	EXPECT_NEAR(N.Pitch, 160.0f, T);
}

TEST(FRotatorTest, NormalizeAlreadyNormal)
{
	const FRotator R(45.0f, -90.0f, 179.0f);
	const FRotator N = R.GetNormalized();
	EXPECT_NEAR(N.Pitch, 45.0f, T);
	EXPECT_NEAR(N.Yaw, -90.0f, T);
	EXPECT_NEAR(N.Roll, 179.0f, T);
}

TEST(FRotatorTest, Normalize360)
{
	const FRotator R(360.0f, 720.0f, -360.0f);
	const FRotator N = R.GetNormalized();
	EXPECT_NEAR(N.Pitch, 0.0f, T);
	EXPECT_NEAR(N.Yaw, 0.0f, T);
	EXPECT_NEAR(N.Roll, 0.0f, T);
}

// =================================================================
// Quaternion conversion roundtrip
// =================================================================

TEST(FRotatorTest, QuaternionRoundtripYaw)
{
	const FRotator R(0.0f, 45.0f, 0.0f);
	const FQuat Q = R.Quaternion();
	const FRotator R2 = Q.ToRotator();
	EXPECT_NEAR(R2.Pitch, 0.0f, T);
	EXPECT_NEAR(R2.Yaw, 45.0f, T);
	EXPECT_NEAR(R2.Roll, 0.0f, T);
}

TEST(FRotatorTest, QuaternionRoundtripPitch)
{
	const FRotator R(30.0f, 0.0f, 0.0f);
	const FQuat Q = R.Quaternion();
	const FRotator R2 = Q.ToRotator();
	EXPECT_NEAR(R2.Pitch, 30.0f, T);
	EXPECT_NEAR(R2.Yaw, 0.0f, T);
	EXPECT_NEAR(R2.Roll, 0.0f, T);
}

TEST(FRotatorTest, QuaternionRoundtripRoll)
{
	const FRotator R(0.0f, 0.0f, 60.0f);
	const FQuat Q = R.Quaternion();
	const FRotator R2 = Q.ToRotator();
	EXPECT_NEAR(R2.Pitch, 0.0f, T);
	EXPECT_NEAR(R2.Yaw, 0.0f, T);
	EXPECT_NEAR(R2.Roll, 60.0f, T);
}

TEST(FRotatorTest, QuaternionRoundtripCombined)
{
	// Test combined rotation via rotation effect (Euler angles may differ).
	const FRotator R(20.0f, 35.0f, 15.0f);
	const FQuat Q = R.Quaternion();
	const FRotator R2 = Q.ToRotator();
	const FQuat Q2 = R2.Quaternion();
	// Compare rotation effect on a test vector.
	const FVector V(1.0f, 0.0f, 0.0f);
	const FVector V1 = Q.RotateVector(V);
	const FVector V2 = Q2.RotateVector(V);
	EXPECT_NEAR(V1.X, V2.X, T);
	EXPECT_NEAR(V1.Y, V2.Y, T);
	EXPECT_NEAR(V1.Z, V2.Z, T);
}

// =================================================================
// ToMatrix
// =================================================================

TEST(FRotatorTest, ToMatrixMatchesQuaternionMatrix)
{
	const FRotator R(30.0f, 45.0f, 0.0f);
	const FMatrix M1 = R.ToMatrix();
	const FMatrix M2 = R.Quaternion().ToMatrix();
	EXPECT_TRUE(M1.Equals(M2, T));
}

// =================================================================
// Zero / IsNearlyZero / IsZero
// =================================================================

TEST(FRotatorTest, ZeroRotatorConstant)
{
	EXPECT_EQ(FRotator::ZeroRotator.Pitch, 0.0f);
	EXPECT_EQ(FRotator::ZeroRotator.Yaw, 0.0f);
	EXPECT_EQ(FRotator::ZeroRotator.Roll, 0.0f);
}

TEST(FRotatorTest, IsZero)
{
	EXPECT_TRUE(FRotator(0.0f, 0.0f, 0.0f).IsZero());
	EXPECT_FALSE(FRotator(0.0f, 0.0f, 1.0f).IsZero());
}

TEST(FRotatorTest, IsNearlyZero)
{
	constexpr FRotator R(0.00005f, -0.00005f, 0.00001f);
	EXPECT_TRUE(R.IsNearlyZero());
}

TEST(FRotatorTest, IsNearlyZeroFalse)
{
	constexpr FRotator R(1.0f, 0.0f, 0.0f);
	EXPECT_FALSE(R.IsNearlyZero());
}
