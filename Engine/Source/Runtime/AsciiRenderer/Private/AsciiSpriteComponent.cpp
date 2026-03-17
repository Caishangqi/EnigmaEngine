// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiSpriteComponent.cpp
/// @brief Implementation of FAsciiSpriteComponent.

#include "AsciiRenderer/AsciiSpriteComponent.h"

#include "GameFramework/GameObject.h"
#include "GameFramework/TransformComponent.h"
#include "Modules/ModuleManager.h"
#include "RenderCore/AsciiRendererInterface.h"

#include <cmath>

namespace Enigma
{

void FAsciiSpriteComponent::Render()
{
	FGameObject* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	const FVector pos = owner->GetTransform().GetPosition();

	// Round to nearest cell.
	const int32_t x = static_cast<int32_t>(std::round(pos.X));
	const int32_t y = static_cast<int32_t>(std::round(pos.Y));

	auto& renderer = FModuleManager::Get()
		.GetModuleChecked<IAsciiRendererModule>("Renderer");

	const FAsciiCell cell(DisplayChar, Fg, Bg);
	renderer.FillRect(x, y, Width, Height, ZOrder, cell);
}

} // namespace Enigma
