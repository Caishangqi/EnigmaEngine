// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputTriggerTest.cpp
/// @brief Unit tests for built-in input triggers.

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
#include "InputTriggers.h"
#include "Math/Vector.h"

using namespace Enigma;

// =================================================================
// FInputTriggerDown
// =================================================================

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputTriggerTest, DownTriggeredWhileActuated)
{
	FInputTriggerDown trigger;
	FInputActionValue pressed(true);
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputTriggerTest, DownNoneWhenNotActuated)
{
	FInputTriggerDown trigger;
	FInputActionValue released(false);
	TestEqual("EXPECT_EQ", trigger.UpdateState(released, 0.016f), ETriggerState::None);
}

// =================================================================
// FInputTriggerPressed
// =================================================================

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputTriggerTest, PressedTriggeredOnFirstFrameOnly)
{
	FInputTriggerPressed trigger;
	FInputActionValue pressed(true);
	FInputActionValue released(false);

	// First frame pressed -> Triggered
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
	// Second frame still pressed -> None (already fired)
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.016f), ETriggerState::None);
	// Release
	TestEqual("EXPECT_EQ", trigger.UpdateState(released, 0.016f), ETriggerState::None);
	// Press again -> Triggered
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
}

// =================================================================
// FInputTriggerReleased
// =================================================================

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputTriggerTest, ReleasedTriggeredOnReleaseFrame)
{
	FInputTriggerReleased trigger;
	FInputActionValue pressed(true);
	FInputActionValue released(false);

	// Not actuated initially -> None
	TestEqual("EXPECT_EQ", trigger.UpdateState(released, 0.016f), ETriggerState::None);
	// Press -> None (Released trigger doesn't fire on press)
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.016f), ETriggerState::None);
	// Release -> Triggered
	TestEqual("EXPECT_EQ", trigger.UpdateState(released, 0.016f), ETriggerState::Triggered);
	// Stay released -> None
	TestEqual("EXPECT_EQ", trigger.UpdateState(released, 0.016f), ETriggerState::None);
}

// =================================================================
// FInputTriggerHold
// =================================================================

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputTriggerTest, HoldOngoingThenTriggeredAfterThreshold)
{
	FInputTriggerHold trigger;
	trigger.HoldTimeThreshold = 0.1f; // 100ms
	FInputActionValue pressed(true);
	FInputActionValue released(false);

	// Hold for several frames (each 0.03s) ??should be Ongoing
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);

	// This frame crosses the threshold (0.12s total) -> Triggered
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.03f), ETriggerState::Triggered);

	// After triggering, further holds -> None (one-shot)
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.03f), ETriggerState::None);

	// Release resets
	TestEqual("EXPECT_EQ", trigger.UpdateState(released, 0.03f), ETriggerState::None);

	// Can trigger again after release
	TestEqual("EXPECT_EQ", trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);
}
