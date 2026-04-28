// Copyright EnigmaEngine. All Rights Reserved.

/// @file RenderComponentTests.cpp
/// @brief Unit tests for FRenderComponent auto-registration with Scene.

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

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(RenderComponentTest, OnAttach_RegistersWithScene)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FMockRenderComponent>();

	// RenderScene should call Render on the registered component
	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc->renderCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(RenderComponentTest, OnDetach_UnregistersFromScene)
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

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(RenderComponentTest, RenderScene_CallsAllRegistered)
{
	FScene scene("TestScene");
	auto* obj1 = scene.CreateGameObject("Obj1");
	auto* obj2 = scene.CreateGameObject("Obj2");
	auto* rc1 = obj1->AddComponent<FMockRenderComponent>();
	auto* rc2 = obj2->AddComponent<FMockRenderComponent>();

	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc1->renderCount, 1);
	TestEqual("EXPECT_EQ", rc2->renderCount, 1);

	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc1->renderCount, 2);
	TestEqual("EXPECT_EQ", rc2->renderCount, 2);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(RenderComponentTest, RenderScene_SkipsInactiveOwner)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FMockRenderComponent>();

	obj->SetActive(false);
	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc->renderCount, 0);

	obj->SetActive(true);
	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc->renderCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(RenderComponentTest, RenderScene_SkipsDisabledComponent)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* rc = obj->AddComponent<FMockRenderComponent>();

	rc->SetEnabled(false);
	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc->renderCount, 0);

	rc->SetEnabled(true);
	scene.RenderScene();
	TestEqual("EXPECT_EQ", rc->renderCount, 1);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST(RenderComponentTest, AutoUnregister_OnObjectDestroy)
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
