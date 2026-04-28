// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputModifierTest.cpp
/// @brief Unit tests for built-in input modifiers.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.EnhancedInput." #SuiteName "." #TestName, \
        EnhancedInput, \
        ::Enigma::EAutomationTestType::Unit, \
        ::Enigma::EAutomationTestFlags::None)

#define ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.EnhancedInput." #FixtureName "." #TestName, \
        EnhancedInput, \
        ::Enigma::EAutomationTestType::Unit, \
        ::Enigma::EAutomationTestFlags::None)
#include "InputModifiers.h"
#include "Math/Vector.h"

using namespace Enigma;

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, NegateFlipsAllAxes)
{
	FInputModifierNegate negate;
	// default: bX=true, bY=true, bZ=true
	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = negate.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	TestNear("EXPECT_NEAR", v.X, -1.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Y, -2.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Z, -3.0f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, NegateSelectiveAxis)
{
	FInputModifierNegate negate;
	negate.bX = true;
	negate.bY = false;
	negate.bZ = false;
	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = negate.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	TestNear("EXPECT_NEAR", v.X, -1.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Y, 2.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Z, 3.0f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, ScalarMultiplies)
{
	FInputModifierScalar scalar;
	scalar.Scale = FVector(2.0f, 3.0f, 0.5f);
	FInputActionValue input(FVector(1.0f, 1.0f, 1.0f));
	FInputActionValue result = scalar.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	TestNear("EXPECT_NEAR", v.X, 2.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Y, 3.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Z, 0.5f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, ScaleByDeltaTime)
{
	FInputModifierScaleByDeltaTime mod;
	FInputActionValue input(FVector(10.0f, 20.0f, 0.0f));
	FInputActionValue result = mod.Modify(input, 0.5f);
	FVector v = result.Get<FVector>();
	TestNear("EXPECT_NEAR", v.X, 5.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Y, 10.0f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, DeadZoneBelowThresholdIsZero)
{
	FInputModifierDeadZone dz;
	dz.LowerThreshold = 0.2f;
	dz.UpperThreshold = 1.0f;
	dz.Type = EDeadZoneType::Axial;

	FInputActionValue input(FVector(0.1f, 0.0f, 0.0f));
	FInputActionValue result = dz.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	TestNear("EXPECT_NEAR", v.X, 0.0f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, DeadZoneAboveThresholdRemapped)
{
	FInputModifierDeadZone dz;
	dz.LowerThreshold = 0.2f;
	dz.UpperThreshold = 1.0f;
	dz.Type = EDeadZoneType::Axial;

	FInputActionValue input(FVector(0.6f, 0.0f, 0.0f));
	FInputActionValue result = dz.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	// 0.6 remapped from [0.2, 1.0] to [0, 1]: (0.6-0.2)/(1.0-0.2) = 0.5
	TestNear("EXPECT_NEAR", v.X, 0.5f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, SwizzleAxisYXZ)
{
	FInputModifierSwizzleAxis swizzle;
	swizzle.Order = ESwizzleAxis::YXZ;

	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = swizzle.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	TestNear("EXPECT_NEAR", v.X, 2.0f, 1e-5f); // Y -> X
	TestNear("EXPECT_NEAR", v.Y, 1.0f, 1e-5f); // X -> Y
	TestNear("EXPECT_NEAR", v.Z, 3.0f, 1e-5f); // Z unchanged
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputModifierTest, SwizzleAxisZYX)
{
	FInputModifierSwizzleAxis swizzle;
	swizzle.Order = ESwizzleAxis::ZYX;

	FInputActionValue input(FVector(1.0f, 2.0f, 3.0f));
	FInputActionValue result = swizzle.Modify(input, 0.016f);
	FVector v = result.Get<FVector>();
	TestNear("EXPECT_NEAR", v.X, 3.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Y, 2.0f, 1e-5f);
	TestNear("EXPECT_NEAR", v.Z, 1.0f, 1e-5f);
}
