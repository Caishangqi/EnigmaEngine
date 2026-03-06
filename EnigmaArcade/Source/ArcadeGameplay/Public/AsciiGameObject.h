// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "ArcadeGameplayAPI.generated.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

#include <cmath>
#include <cstdint>

namespace Enigma
{

class IAsciiRendererModule;

// ---------------------------------------------------------------
// FAsciiGameObject -- lightweight ASCII game object
//
// Stores float position for frame-rate independent movement.
// Velocity is applied in Update() then reset to zero, so input
// callbacks must set velocity every frame the key is held.
//
// Render() rounds position to integer and draws via FillRect.
// ---------------------------------------------------------------
class ARCADEGAMEPLAY_API FAsciiGameObject
{
public:
	FAsciiGameObject() = default;

	FAsciiGameObject(char displayChar, int32_t width, int32_t height,
	                 FColor fg = FColor::White, FColor bg = FColor::Black)
		: DisplayChar(displayChar)
		, Width(width), Height(height)
		, Fg(fg), Bg(bg)
	{
	}

	/// Apply velocity * dt to position, then reset velocity to zero.
	void Update(float deltaTime);

	/// Round position to int and draw via renderer.FillRect.
	void Render(IAsciiRendererModule& renderer, int32_t zOrder) const;

	/// Clamp position so the object stays within [0, boundsW) x [0, boundsH).
	void ClampToBounds(int32_t boundsW, int32_t boundsH);

	// --- Position (float for smooth movement) ---
	float PosX = 0.0f;
	float PosY = 0.0f;

	// --- Velocity (cells per second, reset each frame after Update) ---
	float VelX = 0.0f;
	float VelY = 0.0f;

	// --- Size ---
	int32_t Width  = 1;
	int32_t Height = 1;

	// --- Appearance ---
	char   DisplayChar = '@';
	FColor Fg = FColor::White;
	FColor Bg = FColor::Black;
};

} // namespace Enigma
