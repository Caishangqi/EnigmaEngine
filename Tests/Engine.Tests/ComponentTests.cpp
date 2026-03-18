// Copyright EnigmaEngine. All Rights Reserved.

/// @file ComponentTests.cpp
/// @brief Unit tests for FComponent lifecycle.

#include <gtest/gtest.h>

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

TEST(ComponentTest, DefaultState_NotAttached)
{
	FMockComponent comp;
	EXPECT_EQ(comp.GetOwner(), nullptr);
	EXPECT_TRUE(comp.IsEnabled());
	EXPECT_FALSE(comp.HasBegunPlay());
}

TEST(ComponentTest, OnAttach_SetsOwner)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	EXPECT_EQ(comp->GetOwner(), obj);
	EXPECT_EQ(comp->onAttachCount, 1);
}

TEST(ComponentTest, BeginPlay_SetsFlag)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	EXPECT_FALSE(comp->HasBegunPlay());
	scene.BeginPlay(); // dispatches BeginPlay on all components
	EXPECT_TRUE(comp->HasBegunPlay());
	EXPECT_EQ(comp->beginPlayCount, 1);
}

TEST(ComponentTest, BeginPlay_CalledOnlyOnce)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	scene.BeginPlay();
	scene.BeginPlay(); // second call is a no-op
	EXPECT_EQ(comp->beginPlayCount, 1);
}

TEST(ComponentTest, BeginPlay_DynamicObject_ReceivesBeginPlayImmediately)
{
	FScene scene("TestScene");
	scene.BeginPlay(); // scene is now in begun-play state

	// Objects created after BeginPlay receive it immediately (UE5 FinishSpawning)
	auto* obj = scene.CreateGameObject("Late");
	auto* comp = obj->AddComponent<FMockComponent>();
	EXPECT_TRUE(comp->HasBegunPlay());
	EXPECT_EQ(comp->beginPlayCount, 1);
}

TEST(ComponentTest, BeginPlay_CalledRegardlessOfEnabledState)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	comp->SetEnabled(false);
	scene.BeginPlay();
	// BeginPlay is dispatched regardless of enabled state (UE5 pattern)
	EXPECT_TRUE(comp->HasBegunPlay());
	EXPECT_EQ(comp->beginPlayCount, 1);
}

TEST(ComponentTest, GetName_ReturnsStaticName)
{
	FMockComponent comp;
	EXPECT_EQ(comp.GetName(), FMockComponent::GetStaticName());
	EXPECT_EQ(comp.GetName(), FName("MockComponent"));
}
