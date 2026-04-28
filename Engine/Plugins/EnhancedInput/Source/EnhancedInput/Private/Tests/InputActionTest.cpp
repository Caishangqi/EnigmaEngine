// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputActionTest.cpp
/// @brief Unit tests for FInputAction.

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
#include "InputAction.h"

using namespace Enigma;

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionTest, ConstructionDefaults)
{
	FInputAction action("TestAction");
	TestEqual("EXPECT_EQ", action.GetName(), "TestAction");
	TestEqual("EXPECT_EQ", action.GetValueType(), EInputActionValueType::Boolean);
	TestTrue("EXPECT_TRUE", action.GetConsumeInput());
	TestEqual("EXPECT_EQ", action.GetAccumulationBehavior(), EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionTest, Axis2DValueType)
{
	FInputAction action("Move", EInputActionValueType::Axis2D);
	TestEqual("EXPECT_EQ", action.GetValueType(), EInputActionValueType::Axis2D);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionTest, SetConsumeInput)
{
	FInputAction action("Test");
	action.SetConsumeInput(false);
	TestFalse("EXPECT_FALSE", action.GetConsumeInput());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionTest, SetAccumulationBehavior)
{
	FInputAction action("Test");
	action.SetAccumulationBehavior(EInputActionAccumulationBehavior::Cumulative);
	TestEqual("EXPECT_EQ", action.GetAccumulationBehavior(), EInputActionAccumulationBehavior::Cumulative);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputActionTest, TriggersAndModifiersInitiallyEmpty)
{
	FInputAction action("Test");
	TestTrue("EXPECT_TRUE", action.GetTriggers().empty());
	TestTrue("EXPECT_TRUE", action.GetModifiers().empty());
}
