// Copyright EnigmaEngine. All Rights Reserved.

/// @file TransformComponentTests.cpp
/// @brief Unit tests for FTransformComponent.

#include <gtest/gtest.h>

#include "GameFramework/TransformComponent.h"
#include "GameFramework/Scene.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"

using namespace Enigma;

TEST(TransformComponentTest, GetStaticName)
{
	EXPECT_EQ(FTransformComponent::GetStaticName(), FName("TransformComponent"));
}

TEST(TransformComponentTest, DefaultPosition_IsZero)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto pos = obj->GetTransform().GetPosition();
	EXPECT_FLOAT_EQ(pos.X, 0.0f);
	EXPECT_FLOAT_EQ(pos.Y, 0.0f);
	EXPECT_FLOAT_EQ(pos.Z, 0.0f);
}

TEST(TransformComponentTest, DefaultScale_IsOne)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto scale = obj->GetTransform().GetScale();
	EXPECT_FLOAT_EQ(scale.X, 1.0f);
	EXPECT_FLOAT_EQ(scale.Y, 1.0f);
	EXPECT_FLOAT_EQ(scale.Z, 1.0f);
}

TEST(TransformComponentTest, DefaultRotation_IsZero)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto rot = obj->GetTransform().GetRotation();
	EXPECT_FLOAT_EQ(rot.Pitch, 0.0f);
	EXPECT_FLOAT_EQ(rot.Yaw, 0.0f);
	EXPECT_FLOAT_EQ(rot.Roll, 0.0f);
}

TEST(TransformComponentTest, SetGetPosition)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetPosition(FVector(10.0f, 20.0f, 30.0f));
	auto pos = obj->GetTransform().GetPosition();
	EXPECT_FLOAT_EQ(pos.X, 10.0f);
	EXPECT_FLOAT_EQ(pos.Y, 20.0f);
	EXPECT_FLOAT_EQ(pos.Z, 30.0f);
}

TEST(TransformComponentTest, SetGetScale)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetScale(FVector(2.0f, 3.0f, 4.0f));
	auto scale = obj->GetTransform().GetScale();
	EXPECT_FLOAT_EQ(scale.X, 2.0f);
	EXPECT_FLOAT_EQ(scale.Y, 3.0f);
	EXPECT_FLOAT_EQ(scale.Z, 4.0f);
}

TEST(TransformComponentTest, SetGetRotation)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetRotation(FRotator(45.0f, 90.0f, 0.0f));
	auto rot = obj->GetTransform().GetRotation();
	EXPECT_NEAR(rot.Pitch, 45.0f, 0.01f);
	EXPECT_NEAR(rot.Yaw, 90.0f, 0.01f);
	EXPECT_NEAR(rot.Roll, 0.0f, 0.01f);
}

TEST(TransformComponentTest, GetTransform_ReturnsConsistentReference)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetPosition(FVector(5.0f, 0.0f, 0.0f));

	const FTransform& t = obj->GetTransform().GetTransform();
	auto pos = t.GetTranslation();
	EXPECT_FLOAT_EQ(pos.X, 5.0f);
}
