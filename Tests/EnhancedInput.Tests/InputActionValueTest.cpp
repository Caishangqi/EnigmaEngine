// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputActionValueTest.cpp
/// @brief Unit tests for FInputActionValue.

#include <gtest/gtest.h>
#include "InputActionValue.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"

using namespace Enigma;

TEST(InputActionValueTest, DefaultIsZero)
{
	FInputActionValue val;
	EXPECT_FALSE(val.IsNonZero());
}

TEST(InputActionValueTest, BoolConstructor)
{
	FInputActionValue t(true);
	FInputActionValue f(false);
	EXPECT_TRUE(t.IsNonZero());
	EXPECT_FALSE(f.IsNonZero());
	EXPECT_TRUE(t.Get<bool>());
	EXPECT_FALSE(f.Get<bool>());
}

TEST(InputActionValueTest, FloatConstructor)
{
	FInputActionValue val(0.75f);
	EXPECT_TRUE(val.IsNonZero());
	EXPECT_NEAR(val.Get<float>(), 0.75f, 1e-5f);
}

TEST(InputActionValueTest, VectorConstructor)
{
	FVector v(1.0f, 2.0f, 3.0f);
	FInputActionValue val(v);
	FVector result = val.Get<FVector>();
	EXPECT_NEAR(result.X, 1.0f, 1e-5f);
	EXPECT_NEAR(result.Y, 2.0f, 1e-5f);
	EXPECT_NEAR(result.Z, 3.0f, 1e-5f);
}

TEST(InputActionValueTest, Reset)
{
	FInputActionValue val(1.0f);
	EXPECT_TRUE(val.IsNonZero());
	val.Reset();
	EXPECT_FALSE(val.IsNonZero());
}

TEST(InputActionValueTest, PlusEqualsAccumulates)
{
	FInputActionValue a(FVector(1.0f, 0.0f, 0.0f));
	FInputActionValue b(FVector(0.0f, 1.0f, 0.0f));
	a += b;
	FVector result = a.Get<FVector>();
	EXPECT_NEAR(result.X, 1.0f, 1e-5f);
	EXPECT_NEAR(result.Y, 1.0f, 1e-5f);
}
