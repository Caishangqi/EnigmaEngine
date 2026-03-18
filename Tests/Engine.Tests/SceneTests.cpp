// Copyright EnigmaEngine. All Rights Reserved.

/// @file SceneTests.cpp
/// @brief Unit tests for FScene object management and frame loop.

#include <gtest/gtest.h>

#include "GameFramework/Component.h"
#include "GameFramework/RenderComponent.h"
#include "GameFramework/Scene.h"

using namespace Enigma;

// =================================================================
// Mock components
// =================================================================

class FSceneTestComponent : public FComponent
{
public:
	static FName GetStaticName() { return FName("SceneTestComponent"); }
	FName GetName() const override { return GetStaticName(); }

	int updateCount = 0;
	bool begunPlay = false;

	void BeginPlay() override
	{
		FComponent::BeginPlay();
		begunPlay = true;
	}

	void Update(float /*dt*/) override { ++updateCount; }
};

class FSceneTestRenderComp : public FRenderComponent
{
public:
	static FName GetStaticName() { return FName("SceneTestRenderComp"); }
	FName GetName() const override { return GetStaticName(); }
	int renderCount = 0;
	void Render() override { ++renderCount; }
};

// =================================================================
// Tests
// =================================================================

TEST(SceneTest, CreateGameObject_AssignsUniqueIDs)
{
	FScene scene("TestScene");
	auto* a = scene.CreateGameObject("A");
	auto* b = scene.CreateGameObject("B");
	auto* c = scene.CreateGameObject("C");
	EXPECT_NE(a->GetID(), b->GetID());
	EXPECT_NE(b->GetID(), c->GetID());
	EXPECT_NE(a->GetID(), c->GetID());
}

TEST(SceneTest, CreateGameObject_SetsScenePointer)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	EXPECT_EQ(obj->GetScene(), &scene);
}

TEST(SceneTest, FindGameObject_ByName)
{
	FScene scene("TestScene");
	scene.CreateGameObject("Alpha");
	scene.CreateGameObject("Beta");
	auto* found = scene.FindGameObject("Beta");
	ASSERT_NE(found, nullptr);
	EXPECT_EQ(found->GetName(), "Beta");
}

TEST(SceneTest, FindGameObject_ReturnsNullIfNotFound)
{
	FScene scene("TestScene");
	scene.CreateGameObject("Alpha");
	EXPECT_EQ(scene.FindGameObject("Missing"), nullptr);
}

TEST(SceneTest, FindGameObjectByID)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	uint64_t id = obj->GetID();
	EXPECT_EQ(scene.FindGameObjectByID(id), obj);
	EXPECT_EQ(scene.FindGameObjectByID(9999), nullptr);
}

TEST(SceneTest, DestroyGameObject_Deferred)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	uint64_t id = obj->GetID();

	scene.DestroyGameObject(id);
	// Object still exists before Tick processes pending destroys
	EXPECT_NE(scene.FindGameObjectByID(id), nullptr);

	scene.Tick(0.0f);
	// Now it should be gone
	EXPECT_EQ(scene.FindGameObjectByID(id), nullptr);
}

TEST(SceneTest, BeginPlay_DrivesComponentBeginPlay)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FSceneTestComponent>();

	EXPECT_FALSE(comp->begunPlay);
	scene.BeginPlay();
	EXPECT_TRUE(comp->begunPlay);
}

TEST(SceneTest, GetAllGameObjects)
{
	FScene scene("TestScene");
	scene.CreateGameObject("A");
	scene.CreateGameObject("B");
	scene.CreateGameObject("C");

	auto all = scene.GetAllGameObjects();
	EXPECT_EQ(all.size(), 3u);
}

TEST(SceneTest, RenderScene_CallsRegisteredComponents)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FSceneTestRenderComp>();

	scene.RenderScene();
	scene.RenderScene();
	EXPECT_EQ(rc->renderCount, 2);
}

TEST(SceneTest, GetName_ReturnsSceneName)
{
	FScene scene("MyLevel");
	EXPECT_EQ(scene.GetName(), "MyLevel");
}

TEST(SceneTest, DestroyGameObject_AutoUnregistersRenderComponent)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FSceneTestRenderComp>();
	uint64_t id = obj->GetID();

	scene.DestroyGameObject(id);
	scene.Tick(0.0f);

	// Should not crash -- render component was auto-unregistered on destroy
	scene.RenderScene();
}
