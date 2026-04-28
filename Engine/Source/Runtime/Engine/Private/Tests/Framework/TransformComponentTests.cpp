// Copyright EnigmaEngine. All Rights Reserved.

/// @file TransformComponentTests.cpp
/// @brief Unit tests for FTransformComponent.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.Engine.Framework." #SuiteName "." #TestName, \
        Engine, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#define ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.Engine.Framework." #FixtureName "." #TestName, \
        Engine, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#include "GameFramework/TransformComponent.h"
#include "GameFramework/Scene.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"

using namespace Enigma;

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, GetStaticName)
{
	TestEqual("EXPECT_EQ", FTransformComponent::GetStaticName(), FName("TransformComponent"));
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, DefaultPosition_IsZero)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto pos = obj->GetTransform().GetPosition();
	TestNear("EXPECT_FLOAT_EQ", pos.X, 0.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", pos.Y, 0.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", pos.Z, 0.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, DefaultScale_IsOne)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto scale = obj->GetTransform().GetScale();
	TestNear("EXPECT_FLOAT_EQ", scale.X, 1.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", scale.Y, 1.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", scale.Z, 1.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, DefaultRotation_IsZero)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto rot = obj->GetTransform().GetRotation();
	TestNear("EXPECT_FLOAT_EQ", rot.Pitch, 0.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", rot.Yaw, 0.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", rot.Roll, 0.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, SetGetPosition)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetPosition(FVector(10.0f, 20.0f, 30.0f));
	auto pos = obj->GetTransform().GetPosition();
	TestNear("EXPECT_FLOAT_EQ", pos.X, 10.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", pos.Y, 20.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", pos.Z, 30.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, SetGetScale)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetScale(FVector(2.0f, 3.0f, 4.0f));
	auto scale = obj->GetTransform().GetScale();
	TestNear("EXPECT_FLOAT_EQ", scale.X, 2.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", scale.Y, 3.0f, 1e-6f);
	TestNear("EXPECT_FLOAT_EQ", scale.Z, 4.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, SetGetRotation)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetRotation(FRotator(45.0f, 90.0f, 0.0f));
	auto rot = obj->GetTransform().GetRotation();
	TestNear("EXPECT_NEAR", rot.Pitch, 45.0f, 0.01f);
	TestNear("EXPECT_NEAR", rot.Yaw, 90.0f, 0.01f);
	TestNear("EXPECT_NEAR", rot.Roll, 0.0f, 0.01f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(TransformComponentTest, GetTransform_ReturnsConsistentReference)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->GetTransform().SetPosition(FVector(5.0f, 0.0f, 0.0f));

	const FTransform& t = obj->GetTransform().GetTransform();
	auto pos = t.GetTranslation();
	TestNear("EXPECT_FLOAT_EQ", pos.X, 5.0f, 1e-6f);
}
