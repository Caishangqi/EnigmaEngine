// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputModifierTest.cpp
/// @brief Unit tests for built-in input modifiers.

#include <gtest/gtest.h>
#include "InputModifiers.h"
#include "Math/Vector.h"

using namespace Enigma;

TEST(InputModifierTest, NegateFlipsAllAxes)
{
	FInputModifierNegate negate;
	// default: bX=true, bY=true, bZ=true
	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = negate.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	EXPECT_NEAR(v.X, -1.0f, 1e-5f);
	EXPECT_NEAR(v.Y, -2.0f, 1e-5f);
	EXPECT_NEAR(v.Z, -3.0f, 1e-5f);
}

TEST(InputModifierTest, NegateSelectiveAxis)
{
	FInputModifierNegate negate;
	negate.bX = true;
	negate.bY = false;
	negate.bZ = false;
	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = negate.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	EXPECT_NEAR(v.X, -1.0f, 1e-5f);
	EXPECT_NEAR(v.Y, 2.0f, 1e-5f);
	EXPECT_NEAR(v.Z, 3.0f, 1e-5f);
}

TEST(InputModifierTest, ScalarMultiplies)
{
	FInputModifierScalar scalar;
	scalar.Scale = FVector(2.0f, 3.0f, 0.5f);
	FInputActionValue input(FVector(1.0f, 1.0f, 1.0f));
	FInputActionValue result = scalar.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	EXPECT_NEAR(v.X, 2.0f, 1e-5f);
	EXPECT_NEAR(v.Y, 3.0f, 1e-5f);
	EXPECT_NEAR(v.Z, 0.5f, 1e-5f);
}

TEST(InputModifierTest, ScaleByDeltaTime)
{
	FInputModifierScaleByDeltaTime mod;
	FInputActionValue input(FVector(10.0f, 20.0f, 0.0f));
	FInputActionValue result = mod.Modify(input, 0.5f);
	FVector v = result.Get<FVector>();
	EXPECT_NEAR(v.X, 5.0f, 1e-5f);
	EXPECT_NEAR(v.Y, 10.0f, 1e-5f);
}

TEST(InputModifierTest, DeadZoneBelowThresholdIsZero)
{
	FInputModifierDeadZone dz;
	dz.LowerThreshold = 0.2f;
	dz.UpperThreshold = 1.0f;
	dz.Type = EDeadZoneType::Axial;

	FInputActionValue input(FVector(0.1f, 0.0f, 0.0f));
	FInputActionValue result = dz.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	EXPECT_NEAR(v.X, 0.0f, 1e-5f);
}

TEST(InputModifierTest, DeadZoneAboveThresholdRemapped)
{
	FInputModifierDeadZone dz;
	dz.LowerThreshold = 0.2f;
	dz.UpperThreshold = 1.0f;
	dz.Type = EDeadZoneType::Axial;

	FInputActionValue input(FVector(0.6f, 0.0f, 0.0f));
	FInputActionValue result = dz.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	// 0.6 remapped from [0.2, 1.0] to [0, 1]: (0.6-0.2)/(1.0-0.2) = 0.5
	EXPECT_NEAR(v.X, 0.5f, 1e-5f);
}

TEST(InputModifierTest, SwizzleAxisYXZ)
{
	FInputModifierSwizzleAxis swizzle;
	swizzle.Order = ESwizzleAxis::YXZ;

	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = swizzle.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	EXPECT_NEAR(v.X, 2.0f, 1e-5f); // Y -> X
	EXPECT_NEAR(v.Y, 1.0f, 1e-5f); // X -> Y
	EXPECT_NEAR(v.Z, 3.0f, 1e-5f); // Z unchanged
}

TEST(InputModifierTest, SwizzleAxisZYX)
{
	FInputModifierSwizzleAxis swizzle;
	swizzle.Order = ESwizzleAxis::ZYX;

	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = swizzle.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	EXPECT_NEAR(v.X, 3.0f, 1e-5f);
	EXPECT_NEAR(v.Y, 2.0f, 1e-5f);
	EXPECT_NEAR(v.Z, 1.0f, 1e-5f);
}
