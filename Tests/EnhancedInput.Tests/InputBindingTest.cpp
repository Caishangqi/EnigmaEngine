// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputBindingTest.cpp
/// @brief Unit tests for action binding and callback invocation.

#include <gtest/gtest.h>
#include "InputSubsystem.h"

using namespace Enigma;

TEST(InputBindingTest, BindActionReturnsValidHandle)
{
	FInputSubsystem subsystem;
	FInputAction action("Test");

	FInputActionCallback cb;
	cb.Bind([](const FInputActionInstance&) {});

	FInputBindingHandle handle = subsystem.BindAction(
		&action, ETriggerEvent::Triggered, std::move(cb));

	EXPECT_EQ(handle.Action, &action);
	EXPECT_NE(static_cast<uint8_t>(handle.Event & ETriggerEvent::Triggered),
		static_cast<uint8_t>(ETriggerEvent::None));
}

TEST(InputBindingTest, UnbindActionRemovesBinding)
{
	FInputSubsystem subsystem;
	FInputAction action("Test");

	FInputActionCallback cb;
	cb.Bind([](const FInputActionInstance&) {});

	FInputBindingHandle handle = subsystem.BindAction(
		&action, ETriggerEvent::Triggered, std::move(cb));

	EXPECT_TRUE(subsystem.UnbindAction(handle));
	// Second unbind should return false
	EXPECT_FALSE(subsystem.UnbindAction(handle));
}

TEST(InputBindingTest, BindActionNullActionReturnsInvalid)
{
	FInputSubsystem subsystem;
	FInputActionCallback cb;
	cb.Bind([](const FInputActionInstance&) {});

	FInputBindingHandle handle = subsystem.BindAction(
		nullptr, ETriggerEvent::Triggered, std::move(cb));

	EXPECT_EQ(handle.Action, nullptr);
}

TEST(InputBindingTest, ClearBindingsRemovesAll)
{
	FInputSubsystem subsystem;
	FInputAction action("Test");

	FInputActionCallback cb1, cb2;
	cb1.Bind([](const FInputActionInstance&) {});
	cb2.Bind([](const FInputActionInstance&) {});

	auto h1 = subsystem.BindAction(&action, ETriggerEvent::Triggered, std::move(cb1));
	auto h2 = subsystem.BindAction(&action, ETriggerEvent::Triggered, std::move(cb2));

	subsystem.ClearBindings();

	// Both should fail to unbind now
	EXPECT_FALSE(subsystem.UnbindAction(h1));
	EXPECT_FALSE(subsystem.UnbindAction(h2));
}
