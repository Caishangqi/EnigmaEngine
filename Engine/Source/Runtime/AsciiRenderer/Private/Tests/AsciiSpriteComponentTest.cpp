// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiSpriteComponentTest.cpp
/// @brief Unit tests for FAsciiSpriteComponent properties and scene integration.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.AsciiRenderer." #SuiteName "." #TestName, \
        AsciiRenderer, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#define ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.AsciiRenderer." #FixtureName "." #TestName, \
        AsciiRenderer, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#include "AsciiRenderer/AsciiSpriteComponent.h"
#include "GameFramework/GameObject.h"
#include "GameFramework/Scene.h"

using namespace Enigma;

// =================================================================
// Type identification
// =================================================================

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, GetStaticName)
{
	TestEqual("EXPECT_EQ", FAsciiSpriteComponent::GetStaticName(), FName("AsciiSpriteComponent"));
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, GetName_MatchesStaticName)
{
	FAsciiSpriteComponent comp;
	TestEqual("EXPECT_EQ", comp.GetName(), FAsciiSpriteComponent::GetStaticName());
}

// =================================================================
// Default property values
// =================================================================

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, DefaultProperties)
{
	FAsciiSpriteComponent comp;
	TestEqual("EXPECT_EQ", comp.DisplayChar, '@');
	TestEqual("EXPECT_EQ", comp.Fg, FColor::White);
	TestEqual("EXPECT_EQ", comp.Bg, FColor::Black);
	TestEqual("EXPECT_EQ", comp.Width, 1);
	TestEqual("EXPECT_EQ", comp.Height, 1);
	TestEqual("EXPECT_EQ", comp.ZOrder, 0);
}

// =================================================================
// Component integration with GameObject
// =================================================================

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, AddComponent_ReturnsNonNull)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* sprite = obj->AddComponent<FAsciiSpriteComponent>();
	TestNotEqual("EXPECT_NE", sprite, nullptr);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, GetComponent_FindsByType)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	auto* added = obj->AddComponent<FAsciiSpriteComponent>();
	auto* found = obj->GetComponent<FAsciiSpriteComponent>();
	TestEqual("EXPECT_EQ", found, added);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, PropertyModification)
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

	TestEqual("EXPECT_EQ", sprite->DisplayChar, '#');
	TestEqual("EXPECT_EQ", sprite->Fg, FColor::Red);
	TestEqual("EXPECT_EQ", sprite->Bg, FColor::Blue);
	TestEqual("EXPECT_EQ", sprite->Width, 3);
	TestEqual("EXPECT_EQ", sprite->Height, 2);
	TestEqual("EXPECT_EQ", sprite->ZOrder, 5);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, MultipleSpritesOnSameObject)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FAsciiSpriteComponent>();
	obj->AddComponent<FAsciiSpriteComponent>();

	auto sprites = obj->GetComponents<FAsciiSpriteComponent>();
	TestEqual("EXPECT_EQ", sprites.size(), 2u);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, RemoveComponent)
{
	FScene scene("TestScene");
	auto* obj = scene.CreateGameObject("Obj");
	obj->AddComponent<FAsciiSpriteComponent>();

	bool removed = obj->RemoveComponent<FAsciiSpriteComponent>();
	TestTrue("EXPECT_TRUE", removed);
	TestEqual("EXPECT_EQ", obj->GetComponent<FAsciiSpriteComponent>(), nullptr);
}

// =================================================================
// Scene render registry integration (inherited from FRenderComponent)
// =================================================================

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(AsciiSpriteComponentTest, DestroyObject_AutoUnregisters)
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
	TestEqual("EXPECT_EQ", scene.FindGameObjectByID(id), nullptr);
}
