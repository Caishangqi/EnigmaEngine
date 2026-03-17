// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiSpriteComponent.h
/// @brief Render component that draws a filled rectangle of ASCII characters.

#include "AsciiRendererAPI.generated.h"
#include "GameFramework/RenderComponent.h"
#include "RenderCore/AsciiCell.h"

#include <cstdint>

namespace Enigma
{

/// @brief Render component that draws a filled rectangle of ASCII characters.
///
/// Reads position from the owning FGameObject's FTransformComponent and submits
/// a FillRect draw command to the active IAsciiRendererModule each frame.
///
/// Usage:
///   auto* sprite = obj->AddComponent<FAsciiSpriteComponent>();
///   sprite->DisplayChar = '@';
///   sprite->Fg = FColor::Green;
///   sprite->Width = 2;
///   sprite->Height = 2;
///
/// Unity equivalent: SpriteRenderer
class ASCIIRENDERER_API FAsciiSpriteComponent : public FRenderComponent
{
public:
	// ----- Type Identification -----

	static FName GetStaticName() { return FName("AsciiSpriteComponent"); }
	FName GetName() const override { return GetStaticName(); }

	// ----- Visual Properties -----

	/// Character used to fill the rectangle.
	char DisplayChar = '@';

	/// Foreground color.
	FColor Fg = FColor::White;

	/// Background color.
	FColor Bg = FColor::Black;

	/// Width in cells.
	int32_t Width = 1;

	/// Height in cells.
	int32_t Height = 1;

	/// Draw order (higher = drawn later = on top).
	int32_t ZOrder = 0;

	// ----- Rendering -----

	/// Reads position from owner's transform and submits a FillRect command.
	void Render() override;
};

} // namespace Enigma
