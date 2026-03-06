// Copyright EnigmaEngine. All Rights Reserved.

#include "AsciiGameObject.h"
#include "RenderCore/AsciiRendererInterface.h"

#include <algorithm>
#include <cmath>

namespace Enigma
{

void FAsciiGameObject::Update(float deltaTime)
{
	PosX += VelX * deltaTime;
	PosY += VelY * deltaTime;

	// Reset velocity -- input callbacks set it each frame while held.
	VelX = 0.0f;
	VelY = 0.0f;
}

void FAsciiGameObject::Render(IAsciiRendererModule& renderer, int32_t zOrder) const
{
	int32_t rx = static_cast<int32_t>(std::round(PosX));
	int32_t ry = static_cast<int32_t>(std::round(PosY));

	renderer.FillRect(rx, ry, Width, Height, zOrder,
		FAsciiCell{DisplayChar, Fg, Bg});
}

void FAsciiGameObject::ClampToBounds(int32_t boundsW, int32_t boundsH)
{
	float maxX = static_cast<float>(std::max(0, boundsW - Width));
	float maxY = static_cast<float>(std::max(0, boundsH - Height));
	PosX = std::clamp(PosX, 0.0f, maxX);
	PosY = std::clamp(PosY, 0.0f, maxY);
}

} // namespace Enigma
