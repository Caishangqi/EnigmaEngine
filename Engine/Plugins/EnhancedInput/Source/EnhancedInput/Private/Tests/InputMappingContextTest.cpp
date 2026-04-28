// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputMappingContextTest.cpp
/// @brief Unit tests for FInputMappingContext.

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
#include "InputMappingContext.h"
#include "InputAction.h"

using namespace Enigma;

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputMappingContextTest, Construction)
{
	FInputMappingContext ctx("TestContext");
	TestEqual("EXPECT_EQ", ctx.GetName(), "TestContext");
	TestTrue("EXPECT_TRUE", ctx.GetMappings().empty());
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputMappingContextTest, MapKeyAddsMappings)
{
	FInputAction action("Jump");
	FInputMappingContext ctx("Test");

	ctx.MapKey(&action, EKeys::SpaceBar);
	if (!TestEqual("ASSERT_EQ", ctx.GetMappings().size(), 1u)) { return; }
	TestEqual("EXPECT_EQ", ctx.GetMappings()[0].Action, &action);
	TestEqual("EXPECT_EQ", ctx.GetMappings()[0].Key, EKeys::SpaceBar);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputMappingContextTest, MapKeyReturnsReferenceForChaining)
{
	FInputAction action("Move", EInputActionValueType::Axis2D);
	FInputTriggerDown trigger;
	FInputMappingContext ctx("Test");

	auto& mapping = ctx.MapKey(&action, EKeys::W);
	mapping.Triggers.push_back(&trigger);

	if (!TestEqual("ASSERT_EQ", ctx.GetMappings().size(), 1u)) { return; }
	TestEqual("EXPECT_EQ", ctx.GetMappings()[0].Triggers.size(), 1u);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputMappingContextTest, UnmapKeyRemovesSpecificMapping)
{
	FInputAction action("Move", EInputActionValueType::Axis2D);
	FInputMappingContext ctx("Test");

	ctx.MapKey(&action, EKeys::W);
	ctx.MapKey(&action, EKeys::S);
	if (!TestEqual("ASSERT_EQ", ctx.GetMappings().size(), 2u)) { return; }

	ctx.UnmapKey(&action, EKeys::W);
	if (!TestEqual("ASSERT_EQ", ctx.GetMappings().size(), 1u)) { return; }
	TestEqual("EXPECT_EQ", ctx.GetMappings()[0].Key, EKeys::S);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputMappingContextTest, UnmapActionRemovesAllForAction)
{
	FInputAction moveAction("Move", EInputActionValueType::Axis2D);
	FInputAction jumpAction("Jump");
	FInputMappingContext ctx("Test");

	ctx.MapKey(&moveAction, EKeys::W);
	ctx.MapKey(&moveAction, EKeys::S);
	ctx.MapKey(&jumpAction, EKeys::SpaceBar);
	if (!TestEqual("ASSERT_EQ", ctx.GetMappings().size(), 3u)) { return; }

	ctx.UnmapAction(&moveAction);
	if (!TestEqual("ASSERT_EQ", ctx.GetMappings().size(), 1u)) { return; }
	TestEqual("EXPECT_EQ", ctx.GetMappings()[0].Action, &jumpAction);
}
