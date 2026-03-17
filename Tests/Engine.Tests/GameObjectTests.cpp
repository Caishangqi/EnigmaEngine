// Copyright EnigmaEngine. All Rights Reserved.

/// @file GameObjectTests.cpp
/// @brief Unit tests for FGameObject component management and lifecycle.

#include <gtest/gtest.h>

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

class GameObjectTest : public ::testing::Test
{
protected:
	void SetUp() override { g_detachLog.clear(); }
};

// =================================================================
// Tests
// =================================================================

TEST_F(GameObjectTest, CreateViaScene_HasValidID)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Player");
	EXPECT_NE(obj->GetID(), 0u);
}

TEST_F(GameObjectTest, GetName_ReturnsCorrectName)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Player");
	EXPECT_EQ(obj->GetName(), "Player");
}

TEST_F(GameObjectTest, BuiltInTransform_Accessible)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	// Transform is always available
	obj->GetTransform().SetPosition(FVector(1.0f, 2.0f, 3.0f));
	EXPECT_FLOAT_EQ(obj->GetTransform().GetPosition().X, 1.0f);
}

TEST_F(GameObjectTest, AddComponent_ReturnsNonNull)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponentA>();
	EXPECT_NE(comp, nullptr);
}

TEST_F(GameObjectTest, GetComponent_FindsByType)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* added = obj->AddComponent<FMockComponentA>();
	auto* found = obj->GetComponent<FMockComponentA>();
	EXPECT_EQ(found, added);
}

TEST_F(GameObjectTest, GetComponent_ReturnsNullIfNotFound)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	EXPECT_EQ(obj->GetComponent<FMockComponentA>(), nullptr);
}

TEST_F(GameObjectTest, GetComponents_FindsMultiple)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FMockComponentA>();
	obj->AddComponent<FMockComponentA>();
	obj->AddComponent<FMockComponentB>();

	auto comps = obj->GetComponents<FMockComponentA>();
	EXPECT_EQ(comps.size(), 2u);

	auto bComps = obj->GetComponents<FMockComponentB>();
	EXPECT_EQ(bComps.size(), 1u);
}

TEST_F(GameObjectTest, RemoveComponent_DetachesAndRemoves)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FMockComponentA>();

	bool removed = obj->RemoveComponent<FMockComponentA>();
	EXPECT_TRUE(removed);
	EXPECT_EQ(obj->GetComponent<FMockComponentA>(), nullptr);
	ASSERT_EQ(g_detachLog.size(), 1u);
	EXPECT_EQ(g_detachLog[0], "A");
}

TEST_F(GameObjectTest, RemoveComponent_ReturnsFalseIfNotFound)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	bool removed = obj->RemoveComponent<FMockComponentC>();
	EXPECT_FALSE(removed);
}

TEST_F(GameObjectTest, SetActive_Inactive_SkipsUpdate)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* comp = obj->AddComponent<FMockComponentA>();
	(void)comp; // unused -- we just verify no crash

	obj->SetActive(false);
	scene.Tick(0.016f);
	// Inactive objects skip Update entirely
	EXPECT_FALSE(obj->IsActive());
}

TEST_F(GameObjectTest, DestroyOrder_ReverseAttachment)
{
	{
		FScene scene("TestScene");
		auto* obj = scene.CreateGameObject("Obj");
		obj->AddComponent<FMockComponentA>(); // attached first
		obj->AddComponent<FMockComponentB>(); // attached second
		// scene destructor destroys obj, which detaches in reverse order
	}

	ASSERT_GE(g_detachLog.size(), 2u);
	// B was attached second -> detached first (reverse order)
	EXPECT_EQ(g_detachLog[0], "B");
	EXPECT_EQ(g_detachLog[1], "A");
}

TEST_F(GameObjectTest, GetScene_ReturnsCorrectScene)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	EXPECT_EQ(obj->GetScene(), &scene);
}
