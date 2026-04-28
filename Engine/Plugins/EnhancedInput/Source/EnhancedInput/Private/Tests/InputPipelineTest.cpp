// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputPipelineTest.cpp
/// @brief End-to-end tests for the input processing pipeline.
/// Tests: SetKeyState -> Tick -> modifiers -> triggers -> callback.

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
#include "InputSubsystem.h"
#include "Math/Vector.h"

using namespace Enigma;

class InputPipelineTest : public ::Enigma::FAutomationTestFixture
{
protected:
	FInputSubsystem subsystem;
	FInputAction moveAction{"Move", EInputActionValueType::Axis2D};
	FInputMappingContext context{"TestContext"};
	FInputTriggerDown downTrigger;
	FInputModifierNegate negateX;
	FInputModifierSwizzleAxis swizzleYXZ;

	void SetUp() override
	{
		moveAction.SetAccumulationBehavior(
			EInputActionAccumulationBehavior::Cumulative);
		negateX.bX = true;
		negateX.bY = false;
		negateX.bZ = false;
		swizzleYXZ.Order = ESwizzleAxis::YXZ;
	}
};

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(InputPipelineTest, SingleKeyProducesValue)
{
	// D -> +X
	context.MapKey(&moveAction, EKeys::D)
		.Triggers.push_back(&downTrigger);
	subsystem.AddMappingContext(&context, 0);

	FVector received;
	bool called = false;
	FInputActionCallback cb;
	cb.Bind([&](const FInputActionInstance& inst)
	{
		received = inst.Value.Get<FVector>();
		called = true;
	});
	subsystem.BindAction(&moveAction,
		ETriggerEvent::Triggered, std::move(cb));

	// Press D and tick
	subsystem.SetKeyState(EKeys::D, true);
	subsystem.Tick(0.016f);

	TestTrue("EXPECT_TRUE", called);
	TestNear("EXPECT_NEAR", received.X, 1.0f, 1e-5f);
	TestNear("EXPECT_NEAR", received.Y, 0.0f, 1e-5f);
}
ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(InputPipelineTest, ModifiersAppliedCorrectly)
{
	// S -> negate X then swizzle YXZ -> (0, -1, 0)
	auto& mapping = context.MapKey(&moveAction, EKeys::S);
	mapping.Modifiers.push_back(&negateX);
	mapping.Modifiers.push_back(&swizzleYXZ);
	mapping.Triggers.push_back(&downTrigger);
	subsystem.AddMappingContext(&context, 0);

	FVector received;
	FInputActionCallback cb;
	cb.Bind([&](const FInputActionInstance& inst)
	{
		received = inst.Value.Get<FVector>();
	});
	subsystem.BindAction(&moveAction,
		ETriggerEvent::Triggered, std::move(cb));

	subsystem.SetKeyState(EKeys::S, true);
	subsystem.Tick(0.016f);

	TestNear("EXPECT_NEAR", received.X, 0.0f, 1e-5f);
	TestNear("EXPECT_NEAR", received.Y, -1.0f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(InputPipelineTest, CumulativeAccumulation)
{
	// W -> +Y, D -> +X (both pressed = diagonal)
	auto& wMap = context.MapKey(&moveAction, EKeys::W);
	wMap.Modifiers.push_back(&swizzleYXZ);
	wMap.Triggers.push_back(&downTrigger);

	context.MapKey(&moveAction, EKeys::D)
		.Triggers.push_back(&downTrigger);

	subsystem.AddMappingContext(&context, 0);

	FVector received;
	FInputActionCallback cb;
	cb.Bind([&](const FInputActionInstance& inst)
	{
		received = inst.Value.Get<FVector>();
	});
	subsystem.BindAction(&moveAction,
		ETriggerEvent::Triggered, std::move(cb));

	// Press both W and D
	subsystem.SetKeyState(EKeys::W, true);
	subsystem.SetKeyState(EKeys::D, true);
	subsystem.Tick(0.016f);

	// Cumulative: (1,0,0) + (0,1,0) = (1,1,0)
	TestNear("EXPECT_NEAR", received.X, 1.0f, 1e-5f);
	TestNear("EXPECT_NEAR", received.Y, 1.0f, 1e-5f);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(InputPipelineTest, NoCallbackWhenKeyReleased)
{
	context.MapKey(&moveAction, EKeys::D)
		.Triggers.push_back(&downTrigger);
	subsystem.AddMappingContext(&context, 0);

	int callCount = 0;
	FInputActionCallback cb;
	cb.Bind([&](const FInputActionInstance&) { ++callCount; });
	subsystem.BindAction(&moveAction,
		ETriggerEvent::Triggered, std::move(cb));

	// Press and tick -> callback fires
	subsystem.SetKeyState(EKeys::D, true);
	subsystem.Tick(0.016f);
	TestEqual("EXPECT_EQ", callCount, 1);

	// Release and tick -> no Triggered event (Completed instead)
	subsystem.SetKeyState(EKeys::D, false);
	subsystem.Tick(0.016f);
	TestEqual("EXPECT_EQ", callCount, 1); // still 1
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(InputPipelineTest, PressedTriggerFiresOnce)
{
	FInputTriggerPressed pressedTrigger;
	FInputAction jumpAction("Jump");

	FInputMappingContext jumpCtx("JumpCtx");
	jumpCtx.MapKey(&jumpAction, EKeys::SpaceBar)
		.Triggers.push_back(&pressedTrigger);
	subsystem.AddMappingContext(&jumpCtx, 0);

	int callCount = 0;
	FInputActionCallback cb;
	cb.Bind([&](const FInputActionInstance&) { ++callCount; });
	subsystem.BindAction(&jumpAction,
		ETriggerEvent::Triggered, std::move(cb));

	// Press space
	subsystem.SetKeyState(EKeys::SpaceBar, true);
	subsystem.Tick(0.016f);
	TestEqual("EXPECT_EQ", callCount, 1);

	// Hold space ??should not fire again
	subsystem.Tick(0.016f);
	TestEqual("EXPECT_EQ", callCount, 1);

	// Release and press again
	subsystem.SetKeyState(EKeys::SpaceBar, false);
	subsystem.Tick(0.016f);
	subsystem.SetKeyState(EKeys::SpaceBar, true);
	subsystem.Tick(0.016f);
	TestEqual("EXPECT_EQ", callCount, 2);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST_F(InputPipelineTest, ContextSwitchChangesActiveMappings)
{
	FInputAction resizeAction("Resize", EInputActionValueType::Axis2D);
	resizeAction.SetAccumulationBehavior(
		EInputActionAccumulationBehavior::Cumulative);
	FInputTriggerPressed pressedTrigger;

	FInputMappingContext resizeCtx("ResizeCtx");
	resizeCtx.MapKey(&resizeAction, EKeys::D)
		.Triggers.push_back(&pressedTrigger);

	// Start with move context
	context.MapKey(&moveAction, EKeys::D)
		.Triggers.push_back(&downTrigger);
	subsystem.AddMappingContext(&context, 0);

	int moveCount = 0, resizeCount = 0;
	FInputActionCallback moveCb, resizeCb;
	moveCb.Bind([&](const FInputActionInstance&) { ++moveCount; });
	resizeCb.Bind([&](const FInputActionInstance&) { ++resizeCount; });
	subsystem.BindAction(&moveAction,
		ETriggerEvent::Triggered, std::move(moveCb));
	subsystem.BindAction(&resizeAction,
		ETriggerEvent::Triggered, std::move(resizeCb));

	// Press D in move context
	subsystem.SetKeyState(EKeys::D, true);
	subsystem.Tick(0.016f);
	TestEqual("EXPECT_EQ", moveCount, 1);
	TestEqual("EXPECT_EQ", resizeCount, 0);

	// Switch to resize context
	subsystem.RemoveMappingContext(&context);
	subsystem.AddMappingContext(&resizeCtx, 0);

	// Release and press D again in resize context
	subsystem.SetKeyState(EKeys::D, false);
	subsystem.Tick(0.016f);
	subsystem.SetKeyState(EKeys::D, true);
	subsystem.Tick(0.016f);
	TestEqual("EXPECT_EQ", resizeCount, 1);
}
