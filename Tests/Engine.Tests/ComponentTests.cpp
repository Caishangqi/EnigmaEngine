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
	scene.Tick(0.016f); // triggers BeginPlay + Update
	EXPECT_TRUE(comp->HasBegunPlay());
	EXPECT_EQ(comp->beginPlayCount, 1);
}

TEST(ComponentTest, BeginPlay_CalledOnlyOnce)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	scene.Tick(0.016f);
	scene.Tick(0.016f);
	scene.Tick(0.016f);
	EXPECT_EQ(comp->beginPlayCount, 1);
	EXPECT_EQ(comp->updateCount, 3);
}

TEST(ComponentTest, Update_ReceivesDeltaTime)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	scene.Tick(0.033f);
	EXPECT_FLOAT_EQ(comp->lastDeltaTime, 0.033f);
}

TEST(ComponentTest, SetEnabled_DisablesUpdate)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponent>();

	comp->SetEnabled(false);
	scene.Tick(0.016f);
	EXPECT_EQ(comp->updateCount, 0);
	EXPECT_FALSE(comp->HasBegunPlay());

	comp->SetEnabled(true);
	scene.Tick(0.016f);
	EXPECT_EQ(comp->updateCount, 1);
	EXPECT_TRUE(comp->HasBegunPlay());
}

TEST(ComponentTest, GetName_ReturnsStaticName)
{
	FMockComponent comp;
	EXPECT_EQ(comp.GetName(), FMockComponent::GetStaticName());
	EXPECT_EQ(comp.GetName(), FName("MockComponent"));
}
