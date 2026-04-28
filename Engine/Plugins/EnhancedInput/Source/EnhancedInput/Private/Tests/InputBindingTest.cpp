// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputBindingTest.cpp
/// @brief Unit tests for action binding and callback invocation.

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

using namespace Enigma;

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputBindingTest, BindActionReturnsValidHandle)
{
	FInputSubsystem subsystem;
	FInputAction action("Test");

	FInputActionCallback cb;
	cb.Bind([](const FInputActionInstance&) {});

	FInputBindingHandle handle = subsystem.BindAction(
		&action, ETriggerEvent::Triggered, std::move(cb));

	TestEqual("EXPECT_EQ", handle.Action, &action);
	TestNotEqual("EXPECT_NE", static_cast<uint8_t>(handle.Event & ETriggerEvent::Triggered), static_cast<uint8_t>(ETriggerEvent::None));
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputBindingTest, UnbindActionRemovesBinding)
{
	FInputSubsystem subsystem;
	FInputAction action("Test");

	FInputActionCallback cb;
	cb.Bind([](const FInputActionInstance&) {});

	FInputBindingHandle handle = subsystem.BindAction(
		&action, ETriggerEvent::Triggered, std::move(cb));

	TestTrue("EXPECT_TRUE", subsystem.UnbindAction(handle));
	// Second unbind should return false
	TestFalse("EXPECT_FALSE", subsystem.UnbindAction(handle));
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputBindingTest, BindActionNullActionReturnsInvalid)
{
	FInputSubsystem subsystem;
	FInputActionCallback cb;
	cb.Bind([](const FInputActionInstance&) {});

	FInputBindingHandle handle = subsystem.BindAction(
		nullptr, ETriggerEvent::Triggered, std::move(cb));

	TestEqual("EXPECT_EQ", handle.Action, nullptr);
}

ENIGMA_IMPLEMENT_ENHANCED_INPUT_AUTOMATION_TEST(InputBindingTest, ClearBindingsRemovesAll)
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
	TestFalse("EXPECT_FALSE", subsystem.UnbindAction(h1));
	TestFalse("EXPECT_FALSE", subsystem.UnbindAction(h2));
}
