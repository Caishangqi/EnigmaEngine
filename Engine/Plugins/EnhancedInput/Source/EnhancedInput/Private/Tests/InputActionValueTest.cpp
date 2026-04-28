// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputActionValueTest.cpp
/// @brief Unit tests for FInputActionValue.

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
#include "InputActionValue.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"

using namespace Enigma;

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionValueTest, DefaultIsZero)
{
	FInputActionValue val;
	TestFalse("EXPECT_FALSE", val.IsNonZero());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionValueTest, BoolConstructor)
{
	FInputActionValue t(true);
	FInputActionValue f(false);
	TestTrue("EXPECT_TRUE", t.IsNonZero());
	TestFalse("EXPECT_FALSE", f.IsNonZero());
	TestTrue("EXPECT_TRUE", t.Get<bool>());
	TestFalse("EXPECT_FALSE", f.Get<bool>());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionValueTest, FloatConstructor)
{
	FInputActionValue val(0.75f);
	TestTrue("EXPECT_TRUE", val.IsNonZero());
	TestNear("EXPECT_NEAR", val.Get<float>(), 0.75f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionValueTest, VectorConstructor)
{
	FVector v(1.0f, 2.0f, 3.0f);
	FInputActionValue val(v);
	FVector result = val.Get<FVector>();
	TestNear("EXPECT_NEAR", result.X, 1.0f, 1e-5f);
	TestNear("EXPECT_NEAR", result.Y, 2.0f, 1e-5f);
	TestNear("EXPECT_NEAR", result.Z, 3.0f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionValueTest, Reset)
{
	FInputActionValue val(1.0f);
	TestTrue("EXPECT_TRUE", val.IsNonZero());
	val.Reset();
	TestFalse("EXPECT_FALSE", val.IsNonZero());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionValueTest, PlusEqualsAccumulates)
{
	FInputActionValue a(FVector(1.0f, 0.0f, 0.0f));
	FInputActionValue b(FVector(0.0f, 1.0f, 0.0f));
	a += b;
	FVector result = a.Get<FVector>();
	TestNear("EXPECT_NEAR", result.X, 1.0f, 1e-5f);
	TestNear("EXPECT_NEAR", result.Y, 1.0f, 1e-5f);
}
