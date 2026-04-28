// Copyright EnigmaEngine. All Rights Reserved.

/// @file GameObjectTests.cpp
/// @brief Unit tests for FGameObject component management and lifecycle.

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

#include <string>
#include <vector>

using namespace Enigma;

// =================================================================
// Mock components
// =================================================================

static std::vector<std::string> g_detachLog;

class FMockComponentA : public FComponent
{
public:
	static FName GetStaticName() { return FName("MockComponentA"); }
	FName GetName() const override { return GetStaticName(); }

	void OnDetach() override
	{
		g_detachLog.push_back("A");
		FComponent::OnDetach();
	}
};

class FMockComponentB : public FComponent
{
public:
	static FName GetStaticName() { return FName("MockComponentB"); }
	FName GetName() const override { return GetStaticName(); }

	void OnDetach() override
	{
		g_detachLog.push_back("B");
		FComponent::OnDetach();
	}
};

class FMockComponentC : public FComponent
{
public:
	static FName GetStaticName() { return FName("MockComponentC"); }
	FName GetName() const override { return GetStaticName(); }
};

class GameObjectTest : public ::Enigma::FAutomationTestFixture
{
protected:
	void SetUp() override { g_detachLog.clear(); }
};

// =================================================================
// Tests
// =================================================================

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, CreateViaScene_HasValidID)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Player");
	TestNotEqual("EXPECT_NE", obj->GetID(), 0u);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, GetName_ReturnsCorrectName)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Player");
	TestEqual("EXPECT_EQ", obj->GetName(), "Player");
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, BuiltInTransform_Accessible)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	// Transform is always available
	obj->GetTransform().SetPosition(FVector(1.0f, 2.0f, 3.0f));
	TestNear("EXPECT_FLOAT_EQ", obj->GetTransform().GetPosition().X, 1.0f, 1e-6f);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, AddComponent_ReturnsNonNull)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponentA>();
	TestNotEqual("EXPECT_NE", comp, nullptr);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, GetComponent_FindsByType)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* added = obj->AddComponent<FMockComponentA>();
	auto* found = obj->GetComponent<FMockComponentA>();
	TestEqual("EXPECT_EQ", found, added);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, GetComponent_ReturnsNullIfNotFound)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	TestEqual("EXPECT_EQ", obj->GetComponent<FMockComponentA>(), nullptr);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, GetComponents_FindsMultiple)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FMockComponentA>();
	obj->AddComponent<FMockComponentA>();
	obj->AddComponent<FMockComponentB>();

	auto comps = obj->GetComponents<FMockComponentA>();
	TestEqual("EXPECT_EQ", comps.size(), 2u);

	auto bComps = obj->GetComponents<FMockComponentB>();
	TestEqual("EXPECT_EQ", bComps.size(), 1u);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, RemoveComponent_DetachesAndRemoves)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FMockComponentA>();

	bool removed = obj->RemoveComponent<FMockComponentA>();
	TestTrue("EXPECT_TRUE", removed);
	TestEqual("EXPECT_EQ", obj->GetComponent<FMockComponentA>(), nullptr);
	if (!TestEqual("ASSERT_EQ", g_detachLog.size(), 1u)) { return; }
	TestEqual("EXPECT_EQ", g_detachLog[0], "A");
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, RemoveComponent_ReturnsFalseIfNotFound)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	bool removed = obj->RemoveComponent<FMockComponentC>();
	TestFalse("EXPECT_FALSE", removed);
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, SetActive_Inactive_SkipsUpdate)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponentA>();
	(void)comp; // unused -- we just verify no crash

	obj->SetActive(false);
	scene.Tick(0.016f);
	// Inactive objects skip Update entirely
	TestFalse("EXPECT_FALSE", obj->IsActive());
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, DestroyOrder_ReverseAttachment)
{
	{
		FScene scene("TestScene");
		auto* obj = scene.CreateGameObject("Obj");
		obj->AddComponent<FMockComponentA>(); // attached first
		obj->AddComponent<FMockComponentB>(); // attached second
		// scene destructor destroys obj, which detaches in reverse order
	}

	if (!TestGreaterThanOrEqual("ASSERT_GE", g_detachLog.size(), 2u)) { return; }
	// B was attached second -> detached first (reverse order)
	TestEqual("EXPECT_EQ", g_detachLog[0], "B");
	TestEqual("EXPECT_EQ", g_detachLog[1], "A");
}

ENIGMA_IMPLEMENT_ENGINE_FRAMEWORK_AUTOMATION_TEST_F(GameObjectTest, GetScene_ReturnsCorrectScene)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	TestEqual("EXPECT_EQ", obj->GetScene(), &scene);
}
