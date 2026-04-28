// Copyright EnigmaEngine. All Rights Reserved.

/// @file FTransformTest.cpp
/// @brief Unit tests for FTransform.

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
#include "Math/Transform.h"
#include "Math/Matrix.h"

using Enigma::FTransform;
using Enigma::FQuat;
using Enigma::FVector;
using Enigma::FMatrix;
using Enigma::FMath;

static constexpr float T = 1e-4f;

// =================================================================
// Identity
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, DefaultIsIdentity)
{
	constexpr FTransform Tf;
	TestTrue("EXPECT_TRUE", Tf.GetRotation().Equals(FQuat::Identity));
	TestEqual("EXPECT_EQ", Tf.GetTranslation().X, 0.0f);
	TestEqual("EXPECT_EQ", Tf.GetTranslation().Y, 0.0f);
	TestEqual("EXPECT_EQ", Tf.GetTranslation().Z, 0.0f);
	TestEqual("EXPECT_EQ", Tf.GetScale3D().X, 1.0f);
	TestEqual("EXPECT_EQ", Tf.GetScale3D().Y, 1.0f);
	TestEqual("EXPECT_EQ", Tf.GetScale3D().Z, 1.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, IdentityConstant)
{
	TestTrue("EXPECT_TRUE", FTransform::Identity.Equals(FTransform()));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, IdentityTransformPosition)
{
	const FVector V(3.0f, -5.0f, 7.0f);
	const FVector R = FTransform::Identity.TransformPosition(V);
	TestNear("EXPECT_NEAR", R.X, V.X, T);
	TestNear("EXPECT_NEAR", R.Y, V.Y, T);
	TestNear("EXPECT_NEAR", R.Z, V.Z, T);
}

// =================================================================
// Constructors
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, TranslationOnlyConstructor)
{
	const FTransform Tf(FVector(1.0f, 2.0f, 3.0f));
	TestTrue("EXPECT_TRUE", Tf.GetRotation().Equals(FQuat::Identity));
	TestEqual("EXPECT_EQ", Tf.GetTranslation().X, 1.0f);
	TestEqual("EXPECT_EQ", Tf.GetTranslation().Y, 2.0f);
	TestEqual("EXPECT_EQ", Tf.GetTranslation().Z, 3.0f);
	TestEqual("EXPECT_EQ", Tf.GetScale3D().X, 1.0f);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, FullTRSConstructor)
{
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FTransform Tf(Q, FVector(1.0f, 2.0f, 3.0f), FVector(2.0f, 2.0f, 2.0f));
	TestTrue("EXPECT_TRUE", Tf.GetRotation().Equals(Q, T));
	TestEqual("EXPECT_EQ", Tf.GetTranslation().X, 1.0f);
	TestEqual("EXPECT_EQ", Tf.GetScale3D().X, 2.0f);
}

// =================================================================
// TransformPosition vs TransformVector
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, TransformPositionAppliesTranslation)
{
	const FTransform Tf(FQuat::Identity, FVector(10.0f, 20.0f, 30.0f));
	const FVector R = Tf.TransformPosition(FVector(1.0f, 2.0f, 3.0f));
	TestNear("EXPECT_NEAR", R.X, 11.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 22.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 33.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, TransformVectorIgnoresTranslation)
{
	const FTransform Tf(FQuat::Identity, FVector(100.0f, 200.0f, 300.0f));
	const FVector R = Tf.TransformVector(FVector(1.0f, 2.0f, 3.0f));
	TestNear("EXPECT_NEAR", R.X, 1.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 2.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 3.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, TransformPositionWithScale)
{
	const FTransform Tf(FQuat::Identity, FVector(0.0f, 0.0f, 0.0f), FVector(2.0f, 3.0f, 4.0f));
	const FVector R = Tf.TransformPosition(FVector(1.0f, 1.0f, 1.0f));
	TestNear("EXPECT_NEAR", R.X, 2.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 3.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 4.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, TransformPositionWithRotation)
{
	// 90 degrees around Y: X -> -Z
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FTransform Tf(Q, FVector(0.0f, 0.0f, 0.0f));
	const FVector R = Tf.TransformPosition(FVector(1.0f, 0.0f, 0.0f));
	TestNear("EXPECT_NEAR", R.X, 0.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 0.0f, T);
	TestNear("EXPECT_NEAR", R.Z, -1.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, TransformVectorAppliesScale)
{
	const FTransform Tf(FQuat::Identity, FVector(100.0f, 200.0f, 300.0f),
		FVector(2.0f, 3.0f, 4.0f));
	const FVector R = Tf.TransformVector(FVector(1.0f, 1.0f, 1.0f));
	TestNear("EXPECT_NEAR", R.X, 2.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 3.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 4.0f, T);
}

// =================================================================
// TRS composition order
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, TRSCompositionOrder)
{
	// Scale(2) then Rotate(90Y) then Translate(10,0,0)
	// Point (1,0,0): Scale -> (2,0,0), Rotate 90Y -> (0,0,-2), Translate -> (10,0,-2)
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	const FTransform Tf(Q, FVector(10.0f, 0.0f, 0.0f), FVector(2.0f, 2.0f, 2.0f));
	const FVector R = Tf.TransformPosition(FVector(1.0f, 0.0f, 0.0f));
	TestNear("EXPECT_NEAR", R.X, 10.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 0.0f, T);
	TestNear("EXPECT_NEAR", R.Z, -2.0f, T);
}

// =================================================================
// ToMatrix equivalence
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, ToMatrixMatchesTransformPosition)
{
	const FQuat Q(FVector(1.0f, 0.0f, 0.0f), FMath::DegreesToRadians(30.0f));
	const FTransform Tf(Q, FVector(5.0f, -3.0f, 7.0f), FVector(1.5f, 2.0f, 0.5f));
	const FVector V(1.0f, 2.0f, 3.0f);
	const FVector ByTransform = Tf.TransformPosition(V);
	const FVector ByMatrix = Tf.ToMatrix().TransformPosition(V);
	TestNear("EXPECT_NEAR", ByTransform.X, ByMatrix.X, T);
	TestNear("EXPECT_NEAR", ByTransform.Y, ByMatrix.Y, T);
	TestNear("EXPECT_NEAR", ByTransform.Z, ByMatrix.Z, T);
}

// =================================================================
// Inverse
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, InverseRoundtrip)
{
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FTransform Tf(Q, FVector(3.0f, -7.0f, 11.0f));
	const FTransform Inv = Tf.GetInverse();
	const FTransform R = Tf * Inv;
	TestTrue("EXPECT_TRUE",
		R.GetRotation().Equals(FQuat::Identity, T)
		|| R.GetRotation().Equals(FQuat(-0.0f, -0.0f, -0.0f, -1.0f), T));
	TestNear("EXPECT_NEAR", R.GetTranslation().X, 0.0f, T);
	TestNear("EXPECT_NEAR", R.GetTranslation().Y, 0.0f, T);
	TestNear("EXPECT_NEAR", R.GetTranslation().Z, 0.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, InverseWithScale)
{
	const FQuat Q(FVector(1.0f, 0.0f, 0.0f), FMath::DegreesToRadians(30.0f));
	const FTransform Tf(Q, FVector(5.0f, 0.0f, 0.0f), FVector(2.0f, 2.0f, 2.0f));
	const FTransform Inv = Tf.GetInverse();
	// Applying transform then inverse should return original point.
	const FVector V(1.0f, 2.0f, 3.0f);
	const FVector Transformed = Tf.TransformPosition(V);
	const FVector Back = Inv.TransformPosition(Transformed);
	TestNear("EXPECT_NEAR", Back.X, V.X, T);
	TestNear("EXPECT_NEAR", Back.Y, V.Y, T);
	TestNear("EXPECT_NEAR", Back.Z, V.Z, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, InverseIdentity)
{
	const FTransform Inv = FTransform::Identity.GetInverse();
	TestTrue("EXPECT_TRUE", Inv.Equals(FTransform::Identity, T));
}

// =================================================================
// Combine (operator*)
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, CombineWithIdentity)
{
	const FQuat Q(FVector(0.0f, 0.0f, 1.0f), FMath::DegreesToRadians(60.0f));
	const FTransform Tf(Q, FVector(1.0f, 2.0f, 3.0f));
	const FTransform R = Tf * FTransform::Identity;
	TestTrue("EXPECT_TRUE", R.GetRotation().Equals(Tf.GetRotation(), T));
	TestNear("EXPECT_NEAR", R.GetTranslation().X, 1.0f, T);
	TestNear("EXPECT_NEAR", R.GetTranslation().Y, 2.0f, T);
	TestNear("EXPECT_NEAR", R.GetTranslation().Z, 3.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, CombineOrder)
{
	// A * B: apply A first, then B.
	// A = translate (10,0,0), B = scale 2.
	// Point (1,0,0): A -> (11,0,0), B -> (22,0,0).
	const FTransform A(FQuat::Identity, FVector(10.0f, 0.0f, 0.0f));
	const FTransform B(FQuat::Identity, FVector(0.0f, 0.0f, 0.0f), FVector(2.0f, 2.0f, 2.0f));
	const FTransform AB = A * B;
	const FVector R = AB.TransformPosition(FVector(1.0f, 0.0f, 0.0f));
	TestNear("EXPECT_NEAR", R.X, 22.0f, T);
	TestNear("EXPECT_NEAR", R.Y, 0.0f, T);
	TestNear("EXPECT_NEAR", R.Z, 0.0f, T);
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, CombineMatchesSequentialTransform)
{
	const FQuat QA(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(30.0f));
	const FQuat QB(FVector(1.0f, 0.0f, 0.0f), FMath::DegreesToRadians(45.0f));
	const FTransform A(QA, FVector(1.0f, 2.0f, 3.0f));
	const FTransform B(QB, FVector(-1.0f, 0.0f, 5.0f));
	const FTransform AB = A * B;
	const FVector V(2.0f, -1.0f, 4.0f);
	// Sequential: apply A, then B.
	const FVector Step1 = A.TransformPosition(V);
	const FVector Step2 = B.TransformPosition(Step1);
	const FVector Combined = AB.TransformPosition(V);
	TestNear("EXPECT_NEAR", Combined.X, Step2.X, T);
	TestNear("EXPECT_NEAR", Combined.Y, Step2.Y, T);
	TestNear("EXPECT_NEAR", Combined.Z, Step2.Z, T);
}

// =================================================================
// Getters / Setters
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, SettersWork)
{
	FTransform Tf;
	Tf.SetTranslation(FVector(1.0f, 2.0f, 3.0f));
	Tf.SetScale3D(FVector(4.0f, 5.0f, 6.0f));
	const FQuat Q(FVector(0.0f, 1.0f, 0.0f), FMath::DegreesToRadians(90.0f));
	Tf.SetRotation(Q);
	TestEqual("EXPECT_EQ", Tf.GetTranslation().X, 1.0f);
	TestEqual("EXPECT_EQ", Tf.GetScale3D().Y, 5.0f);
	TestTrue("EXPECT_TRUE", Tf.GetRotation().Equals(Q, T));
}

// =================================================================
// Equals
// =================================================================

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, EqualsWithinTolerance)
{
	const FTransform A;
	FTransform B;
	B.SetTranslation(FVector(1e-5f, 1e-5f, 1e-5f));
	TestTrue("EXPECT_TRUE", A.Equals(B));
}

ENIGMA_IMPLEMENT_CORE_MATH_AUTOMATION_TEST(FTransformTest, EqualsOutsideTolerance)
{
	const FTransform A;
	const FTransform B(FQuat::Identity, FVector(1.0f, 0.0f, 0.0f));
	TestFalse("EXPECT_FALSE", A.Equals(B));
}
