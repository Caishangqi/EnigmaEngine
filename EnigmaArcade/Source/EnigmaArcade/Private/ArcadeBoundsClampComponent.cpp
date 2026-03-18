// Copyright EnigmaEngine. All Rights Reserved.

#include "ArcadeBoundsClampComponent.h"
#include "GameFramework/GameObject.h"
#include "AsciiRenderer/AsciiSpriteComponent.h"
#include "Modules/ModuleManager.h"
#include "RenderCore/AsciiRendererInterface.h"
#include "Math/Vector.h"
#include "TickSystem/TickGroup.h"

#include <algorithm>

FArcadeBoundsClampComponent::FArcadeBoundsClampComponent()
{
	bCanEverTick = true;
	TickGroup = Enigma::ETickGroup::TG_PostUpdate;
}

void FArcadeBoundsClampComponent::Update(float /*deltaTime*/)
{
	if (!GetOwner())
	{
		return;
	}

	// Query sibling sprite for dimensions
	auto* sprite = GetOwner()->GetComponent<Enigma::FAsciiSpriteComponent>();
	if (!sprite)
	{
		return;
	}

	// Query renderer for framebuffer bounds
	auto& renderer = Enigma::FModuleManager::Get()
		.GetModuleChecked<Enigma::IAsciiRendererModule>("Renderer");
	const float maxX = static_cast<float>(renderer.GetFrameBufferWidth()  - sprite->Width);
	const float maxY = static_cast<float>(renderer.GetFrameBufferHeight() - sprite->Height);

	Enigma::FVector pos = GetOwner()->GetTransform().GetPosition();
	pos.X = std::clamp(pos.X, 0.0f, maxX);
	pos.Y = std::clamp(pos.Y, 0.0f, maxY);
	GetOwner()->GetTransform().SetPosition(pos);
}
