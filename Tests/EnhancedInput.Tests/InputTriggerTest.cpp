// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputTriggerTest.cpp
/// @brief Unit tests for built-in input triggers.

#include <gtest/gtest.h>
#include "InputTriggers.h"
#include "Math/Vector.h"

using namespace Enigma;

// =================================================================
// FInputTriggerDown
// =================================================================

TEST(InputTriggerTest, DownTriggeredWhileActuated)
{
	FInputTriggerDown trigger;
	FInputActionValue pressed(true);
	EXPECT_EQ(trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
	EXPECT_EQ(trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
}

TEST(InputTriggerTest, DownNoneWhenNotActuated)
{
	FInputTriggerDown trigger;
	FInputActionValue released(false);
	EXPECT_EQ(trigger.UpdateState(released, 0.016f), ETriggerState::None);
}

// =================================================================
// FInputTriggerPressed
// =================================================================

TEST(InputTriggerTest, PressedTriggeredOnFirstFrameOnly)
{
	FInputTriggerPressed trigger;
	FInputActionValue pressed(true);
	FInputActionValue released(false);

	// First frame pressed -> Triggered
	EXPECT_EQ(trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
	// Second frame still pressed -> None (already fired)
	EXPECT_EQ(trigger.UpdateState(pressed, 0.016f), ETriggerState::None);
	// Release
	EXPECT_EQ(trigger.UpdateState(released, 0.016f), ETriggerState::None);
	// Press again -> Triggered
	EXPECT_EQ(trigger.UpdateState(pressed, 0.016f), ETriggerState::Triggered);
}

// =================================================================
// FInputTriggerReleased
// =================================================================

TEST(InputTriggerTest, ReleasedTriggeredOnReleaseFrame)
{
	FInputTriggerReleased trigger;
	FInputActionValue pressed(true);
	FInputActionValue released(false);

	// Not actuated initially -> None
	EXPECT_EQ(trigger.UpdateState(released, 0.016f), ETriggerState::None);
	// Press -> None (Released trigger doesn't fire on press)
	EXPECT_EQ(trigger.UpdateState(pressed, 0.016f), ETriggerState::None);
	// Release -> Triggered
	EXPECT_EQ(trigger.UpdateState(released, 0.016f), ETriggerState::Triggered);
	// Stay released -> None
	EXPECT_EQ(trigger.UpdateState(released, 0.016f), ETriggerState::None);
}

// =================================================================
// FInputTriggerHold
// =================================================================

TEST(InputTriggerTest, HoldOngoingThenTriggeredAfterThreshold)
{
	FInputTriggerHold trigger;
	trigger.HoldTimeThreshold = 0.1f; // 100ms
	FInputActionValue pressed(true);
	FInputActionValue released(false);

	// Hold for several frames (each 0.03s) — should be Ongoing
	EXPECT_EQ(trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);
	EXPECT_EQ(trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);
	EXPECT_EQ(trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);

	// This frame crosses the threshold (0.12s total) -> Triggered
	EXPECT_EQ(trigger.UpdateState(pressed, 0.03f), ETriggerState::Triggered);

	// After triggering, further holds -> None (one-shot)
	EXPECT_EQ(trigger.UpdateState(pressed, 0.03f), ETriggerState::None);

	// Release resets
	EXPECT_EQ(trigger.UpdateState(released, 0.03f), ETriggerState::None);

	// Can trigger again after release
	EXPECT_EQ(trigger.UpdateState(pressed, 0.03f), ETriggerState::Ongoing);
}
