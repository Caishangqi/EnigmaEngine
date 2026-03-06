// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputActionTest.cpp
/// @brief Unit tests for FInputAction.

#include <gtest/gtest.h>
#include "InputAction.h"

using namespace Enigma;

TEST(InputActionTest, ConstructionDefaults)
{
	FInputAction action("TestAction");
	EXPECT_EQ(action.GetName(), "TestAction");
	EXPECT_EQ(action.GetValueType(), EInputActionValueType::Boolean);
	EXPECT_TRUE(action.GetConsumeInput());
	EXPECT_EQ(action.GetAccumulationBehavior(),
		EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
}

TEST(InputActionTest, Axis2DValueType)
{
	FInputAction action("Move", EInputActionValueType::Axis2D);
	EXPECT_EQ(action.GetValueType(), EInputActionValueType::Axis2D);
}

TEST(InputActionTest, SetConsumeInput)
{
	FInputAction action("Test");
	action.SetConsumeInput(false);
	EXPECT_FALSE(action.GetConsumeInput());
}

TEST(InputActionTest, SetAccumulationBehavior)
{
	FInputAction action("Test");
	action.SetAccumulationBehavior(EInputActionAccumulationBehavior::Cumulative);
	EXPECT_EQ(action.GetAccumulationBehavior(),
		EInputActionAccumulationBehavior::Cumulative);
}

TEST(InputActionTest, TriggersAndModifiersInitiallyEmpty)
{
	FInputAction action("Test");
	EXPECT_TRUE(action.GetTriggers().empty());
	EXPECT_TRUE(action.GetModifiers().empty());
}
