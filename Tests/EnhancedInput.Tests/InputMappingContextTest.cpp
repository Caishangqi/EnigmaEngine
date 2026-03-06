// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputMappingContextTest.cpp
/// @brief Unit tests for FInputMappingContext.

#include <gtest/gtest.h>
#include "InputMappingContext.h"
#include "InputAction.h"

using namespace Enigma;

TEST(InputMappingContextTest, Construction)
{
	FInputMappingContext ctx("TestContext");
	EXPECT_EQ(ctx.GetName(), "TestContext");
	EXPECT_TRUE(ctx.GetMappings().empty());
}

TEST(InputMappingContextTest, MapKeyAddsMappings)
{
	FInputAction action("Jump");
	FInputMappingContext ctx("Test");

	ctx.MapKey(&action, EKeys::SpaceBar);
	ASSERT_EQ(ctx.GetMappings().size(), 1u);
	EXPECT_EQ(ctx.GetMappings()[0].Action, &action);
	EXPECT_EQ(ctx.GetMappings()[0].Key, EKeys::SpaceBar);
}

TEST(InputMappingContextTest, MapKeyReturnsReferenceForChaining)
{
	FInputAction action("Move", EInputActionValueType::Axis2D);
	FInputTriggerDown trigger;
	FInputMappingContext ctx("Test");

	auto& mapping = ctx.MapKey(&action, EKeys::W);
	mapping.Triggers.push_back(&trigger);

	ASSERT_EQ(ctx.GetMappings().size(), 1u);
	EXPECT_EQ(ctx.GetMappings()[0].Triggers.size(), 1u);
}

TEST(InputMappingContextTest, UnmapKeyRemovesSpecificMapping)
{
	FInputAction action("Move", EInputActionValueType::Axis2D);
	FInputMappingContext ctx("Test");

	ctx.MapKey(&action, EKeys::W);
	ctx.MapKey(&action, EKeys::S);
	ASSERT_EQ(ctx.GetMappings().size(), 2u);

	ctx.UnmapKey(&action, EKeys::W);
	ASSERT_EQ(ctx.GetMappings().size(), 1u);
	EXPECT_EQ(ctx.GetMappings()[0].Key, EKeys::S);
}

TEST(InputMappingContextTest, UnmapActionRemovesAllForAction)
{
	FInputAction moveAction("Move", EInputActionValueType::Axis2D);
	FInputAction jumpAction("Jump");
	FInputMappingContext ctx("Test");

	ctx.MapKey(&moveAction, EKeys::W);
	ctx.MapKey(&moveAction, EKeys::S);
	ctx.MapKey(&jumpAction, EKeys::SpaceBar);
	ASSERT_EQ(ctx.GetMappings().size(), 3u);

	ctx.UnmapAction(&moveAction);
	ASSERT_EQ(ctx.GetMappings().size(), 1u);
	EXPECT_EQ(ctx.GetMappings()[0].Action, &jumpAction);
}
