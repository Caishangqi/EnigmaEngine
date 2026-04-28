// Copyright EnigmaEngine. All Rights Reserved.

/// @file SceneTests.cpp
/// @brief Unit tests for FScene object management and frame loop.

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

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, CreateGameObject_AssignsUniqueIDs)
{
	FScene scene("TestScene");
	auto* a = scene.CreateGameObject("A");
	auto* b = scene.CreateGameObject("B");
	auto* c = scene.CreateGameObject("C");
	TestNotEqual("EXPECT_NE", a->GetID(), b->GetID());
	TestNotEqual("EXPECT_NE", b->GetID(), c->GetID());
	TestNotEqual("EXPECT_NE", a->GetID(), c->GetID());
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, CreateGameObject_SetsScenePointer)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	TestEqual("EXPECT_EQ", obj->GetScene(), &scene);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, FindGameObject_ByName)
{
	FScene scene("TestScene");
	scene.CreateGameObject("Alpha");
	scene.CreateGameObject("Beta");
	auto* found = scene.FindGameObject("Beta");
	if (!TestNotEqual("ASSERT_NE", found, nullptr)) { return; }
	TestEqual("EXPECT_EQ", found->GetName(), "Beta");
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, FindGameObject_ReturnsNullIfNotFound)
{
	FScene scene("TestScene");
	scene.CreateGameObject("Alpha");
	TestEqual("EXPECT_EQ", scene.FindGameObject("Missing"), nullptr);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, FindGameObjectByID)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	uint64_t id = obj->GetID();
	TestEqual("EXPECT_EQ", scene.FindGameObjectByID(id), obj);
	TestEqual("EXPECT_EQ", scene.FindGameObjectByID(9999), nullptr);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, DestroyGameObject_Deferred)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	uint64_t id = obj->GetID();

	scene.DestroyGameObject(id);
	// Object still exists before Tick processes pending destroys
	TestNotEqual("EXPECT_NE", scene.FindGameObjectByID(id), nullptr);

	scene.Tick(0.0f);
	// Now it should be gone
	TestEqual("EXPECT_EQ", scene.FindGameObjectByID(id), nullptr);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, BeginPlay_DrivesComponentBeginPlay)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FSceneTestComponent>();

	TestFalse("EXPECT_FALSE", comp->begunPlay);
	scene.BeginPlay();
	TestTrue("EXPECT_TRUE", comp->begunPlay);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, GetAllGameObjects)
{
	FScene scene("TestScene");
	scene.CreateGameObject("A");
	scene.CreateGameObject("B");
	scene.CreateGameObject("C");

	auto all = scene.GetAllGameObjects();
	TestEqual("EXPECT_EQ", all.size(), 3u);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, RenderScene_CallsRegisteredComponents)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FSceneTestRenderComp>();

	scene.RenderScene();
	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc->renderCount, 2);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, GetName_ReturnsSceneName)
{
	FScene scene("MyLevel");
	TestEqual("EXPECT_EQ", scene.GetName(), "MyLevel");
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(SceneTest, DestroyGameObject_AutoUnregistersRenderComponent)
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
