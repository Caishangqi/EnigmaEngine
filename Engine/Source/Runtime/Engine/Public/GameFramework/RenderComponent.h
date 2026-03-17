// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file RenderComponent.h
/// @brief Abstract base class for all renderable components.

#include "GameFramework/Component.h"

namespace Enigma
{

class FScene;

/// @brief Abstract base class for components that submit render data.
///
/// Automatically registers/unregisters with the owning FScene's render registry
/// on attach/detach. FScene::RenderScene() iterates only registered render
/// components, decoupling the render traversal from the Update traversal.
///
/// Derived classes MUST implement Render() to submit draw commands.
///
/// Unity equivalent: Renderer (MeshRenderer, SpriteRenderer, etc.)
class ENGINE_API FRenderComponent : public FComponent
{
public:
	// ----- Type Identification -----

	static FName GetStaticName() { return FName("RenderComponent"); }
	FName GetName() const override { return GetStaticName(); }

	// ----- Lifecycle -----

	/// Registers this component with the owning Scene's render registry.
	void OnAttach(FGameObject* owner) override;

	/// Unregisters from the Scene's render registry before detaching.
	void OnDetach() override;

	// ----- Rendering -----

	/// Submit render data to the renderer. Called by FScene::RenderScene().
	/// Derived classes MUST implement this.
	virtual void Render() = 0;
};

} // namespace Enigma
