// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiSpriteComponentTest.cpp
/// @brief Unit tests for FAsciiSpriteComponent properties and scene integration.

#include <gtest/gtest.h>

#include "AsciiRenderer/AsciiSpriteComponent.h"
#include "GameFramework/GameObject.h"
#include "GameFramework/Scene.h"

using namespace Enigma;

// =================================================================
// Type identification
// =================================================================

TEST(AsciiSpriteComponentTest, GetStaticName)
{
	EXPECT_EQ(FAsciiSpriteComponent::GetStaticName(), FName("AsciiSpriteComponent"));
}

TEST(AsciiSpriteComponentTest, GetName_MatchesStaticName)
{
	FAsciiSpriteComponent comp;
	EXPECT_EQ(comp.GetName(), FAsciiSpriteComponent::GetStaticName());
}

// =================================================================
// Default property values
// =================================================================

TEST(AsciiSpriteComponentTest, DefaultProperties)
{
	FAsciiSpriteComponent comp;
	EXPECT_EQ(comp.DisplayChar, '@');
	EXPECT_EQ(comp.Fg, FColor::White);
	EXPECT_EQ(comp.Bg, FColor::Black);
	EXPECT_EQ(comp.Width, 1);
	EXPECT_EQ(comp.Height, 1);
	EXPECT_EQ(comp.ZOrder, 0);
}

// =================================================================
// Component integration with GameObject
// =================================================================

TEST(AsciiSpriteComponentTest, AddComponent_ReturnsNonNull)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* sprite = obj->AddComponent<FAsciiSpriteComponent>();
	EXPECT_NE(sprite, nullptr);
}

TEST(AsciiSpriteComponentTest, GetComponent_FindsByType)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* added = obj->AddComponent<FAsciiSpriteComponent>();
	auto* found = obj->GetComponent<FAsciiSpriteComponent>();
	EXPECT_EQ(found, added);
}

TEST(AsciiSpriteComponentTest, PropertyModification)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* sprite = obj->AddComponent<FAsciiSpriteComponent>();

	sprite->DisplayChar = '#';
	sprite->Fg = FColor::Red;
	sprite->Bg = FColor::Blue;
	sprite->Width = 3;
	sprite->Height = 2;
	sprite->ZOrder = 5;

	EXPECT_EQ(sprite->DisplayChar, '#');
	EXPECT_EQ(sprite->Fg, FColor::Red);
	EXPECT_EQ(sprite->Bg, FColor::Blue);
	EXPECT_EQ(sprite->Width, 3);
	EXPECT_EQ(sprite->Height, 2);
	EXPECT_EQ(sprite->ZOrder, 5);
}

TEST(AsciiSpriteComponentTest, MultipleSpritesOnSameObject)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FAsciiSpriteComponent>();
	obj->AddComponent<FAsciiSpriteComponent>();

	auto sprites = obj->GetComponents<FAsciiSpriteComponent>();
	EXPECT_EQ(sprites.size(), 2u);
}

TEST(AsciiSpriteComponentTest, RemoveComponent)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FAsciiSpriteComponent>();

	bool removed = obj->RemoveComponent<FAsciiSpriteComponent>();
	EXPECT_TRUE(removed);
	EXPECT_EQ(obj->GetComponent<FAsciiSpriteComponent>(), nullptr);
}

// =================================================================
// Scene render registry integration (inherited from FRenderComponent)
// =================================================================

TEST(AsciiSpriteComponentTest, DestroyObject_AutoUnregisters)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FAsciiSpriteComponent>();
	uint64_t id = obj->GetID();

	scene.DestroyGameObject(id);
	scene.Tick(0.0f);

	// Should not crash -- sprite was auto-unregistered on destroy.
	// RenderScene would call Render() on registered components,
	// but destroyed object's components are already unregistered.
	// (RenderScene itself may assert on missing renderer module,
	//  so we just verify the object is gone.)
	EXPECT_EQ(scene.FindGameObjectByID(id), nullptr);
}
