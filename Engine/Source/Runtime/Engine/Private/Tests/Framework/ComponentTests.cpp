// Copyright EnigmaEngine. All Rights Reserved.

/// @file ComponentTests.cpp
/// @brief Unit tests for FComponent lifecycle.

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

#include "GameFramework/Component.h"
#include "GameFramework/Scene.h"

using namespace Enigma;

// =================================================================
// Mock component for testing lifecycle hooks
// =================================================================

class FMockComponent : public FComponent
{
public:
	static FName GetStaticName() { return FName("MockComponent"); }
	FName GetName() const override { return GetStaticName(); }

	int onAttachCount = 0;
	int beginPlayCount = 0;
	int updateCount = 0;
	int onDetachCount = 0;
	float lastDeltaTime = 0.0f;

	void OnAttach(FGameObject* owner) override
	{
		FComponent::OnAttach(owner);
		++onAttachCount;
	}

	void BeginPlay() override
	{
		FComponent::BeginPlay();
		++beginPlayCount;
	}

	void Update(float deltaTime) override
	{
		++updateCount;
		lastDeltaTime = deltaTime;
	}

	void OnDetach() override
	{
		++onDetachCount;
		FComponent::OnDetach();
	}
};

// =================================================================
// Tests
// =================================================================

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(ComponentTest, DefaultState_NotAttached)
{
	FMockComponent comp;
	TestEqual("EXPECT_EQ", comp.GetOwner(), nullptr);
	TestTrue("EXPECT_TRUE", comp.IsEnabled());
	TestFalse("EXPECT_FALSE", comp.HasBegunPlay());
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(ComponentTest, OnAttach_SetsOwner)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	TestEqual("EXPECT_EQ", comp->GetOwner(), obj);
	TestEqual("EXPECT_EQ", comp->onAttachCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(ComponentTest, BeginPlay_SetsFlag)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	TestFalse("EXPECT_FALSE", comp->HasBegunPlay());
	scene.BeginPlay(); // dispatches BeginPlay on all components
	TestTrue("EXPECT_TRUE", comp->HasBegunPlay());
	TestEqual("EXPECT_EQ", comp->beginPlayCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(ComponentTest, BeginPlay_CalledOnlyOnce)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	scene.BeginPlay();
	scene.BeginPlay(); // second call is a no-op
	TestEqual("EXPECT_EQ", comp->beginPlayCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(ComponentTest, BeginPlay_DynamicObject_ReceivesBeginPlayImmediately)
{
	FScene scene("TestScene");
	scene.BeginPlay(); // scene is now in begun-play state

	// Objects created after BeginPlay receive it immediately (UE5 FinishSpawning)
	auto* obj = scene.CreateGameObject("Late");
	auto* comp = obj->AddComponent<FMockComponent>();
	TestTrue("EXPECT_TRUE", comp->HasBegunPlay());
	TestEqual("EXPECT_EQ", comp->beginPlayCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(ComponentTest, BeginPlay_CalledRegardlessOfEnabledState)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	comp->SetEnabled(false);
	scene.BeginPlay();
	// BeginPlay is dispatched regardless of enabled state (UE5 pattern)
	TestTrue("EXPECT_TRUE", comp->HasBegunPlay());
	TestEqual("EXPECT_EQ", comp->beginPlayCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(ComponentTest, GetName_ReturnsStaticName)
{
	FMockComponent comp;
	TestEqual("EXPECT_EQ", comp.GetName(), FMockComponent::GetStaticName());
	TestEqual("EXPECT_EQ", comp.GetName(), FName("MockComponent"));
}
