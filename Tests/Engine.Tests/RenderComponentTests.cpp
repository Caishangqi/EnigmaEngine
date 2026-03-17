// Copyright EnigmaEngine. All Rights Reserved.

/// @file RenderComponentTests.cpp
/// @brief Unit tests for FRenderComponent auto-registration with Scene.

#include <gtest/gtest.h>

#include "GameFramework/RenderComponent.h"
#include "GameFramework/Scene.h"

using namespace Enigma;

// =================================================================
// Mock render component
// =================================================================

class FMockRenderComponent : public FRenderComponent
{
public:
	static FName GetStaticName() { return FName("MockRenderComponent"); }
	FName GetName() const override { return GetStaticName(); }

	int renderCount = 0;

	void Render() override
	{
		++renderCount;
	}
};

// =================================================================
// Tests
// =================================================================

TEST(RenderComponentTest, OnAttach_RegistersWithScene)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FMockRenderComponent>();

	// RenderScene should call Render on the registered component
	scene.RenderScene();
	EXPECT_EQ(rc->renderCount, 1);
}

TEST(RenderComponentTest, OnDetach_UnregistersFromScene)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FMockRenderComponent>();

	// Remove the component -- should unregister from scene
	obj->RemoveComponent<FMockRenderComponent>();

	// Create a dummy to verify scene still works
	FMockRenderComponent probe;
	// RenderScene should not call anything (no registered components)
	// We verify by checking the scene doesn't crash
	scene.RenderScene();
}

TEST(RenderComponentTest, RenderScene_CallsAllRegistered)
{
	FScene scene("TestScene");
	auto* obj1 = scene.CreateGameObject("Obj1");
	auto* obj2 = scene.CreateGameObject("Obj2");
	auto* rc1 = obj1->AddComponent<FMockRenderComponent>();
	auto* rc2 = obj2->AddComponent<FMockRenderComponent>();

	scene.RenderScene();
	EXPECT_EQ(rc1->renderCount, 1);
	EXPECT_EQ(rc2->renderCount, 1);

	scene.RenderScene();
	EXPECT_EQ(rc1->renderCount, 2);
	EXPECT_EQ(rc2->renderCount, 2);
}

TEST(RenderComponentTest, RenderScene_SkipsInactiveOwner)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FMockRenderComponent>();

	obj->SetActive(false);
	scene.RenderScene();
	EXPECT_EQ(rc->renderCount, 0);

	obj->SetActive(true);
	scene.RenderScene();
	EXPECT_EQ(rc->renderCount, 1);
}

TEST(RenderComponentTest, RenderScene_SkipsDisabledComponent)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FMockRenderComponent>();

	rc->SetEnabled(false);
	scene.RenderScene();
	EXPECT_EQ(rc->renderCount, 0);

	rc->SetEnabled(true);
	scene.RenderScene();
	EXPECT_EQ(rc->renderCount, 1);
}

TEST(RenderComponentTest, AutoUnregister_OnObjectDestroy)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FMockRenderComponent>();
	uint64_t id = obj->GetID();

	scene.DestroyGameObject(id);
	scene.Tick(0.0f); // process pending destroys

	// RenderScene should not crash -- component was auto-unregistered
	scene.RenderScene();
}
