// Copyright EnigmaEngine. All Rights Reserved.

#include "ArcadeMovementComponent.h"
#include "GameFramework/GameObject.h"
#include "Math/Vector.h"
#include "TickSystem/TickGroup.h"

FArcadeMovementComponent::FArcadeMovementComponent()
{
	bCanEverTick = true;
	TickGroup = Enigma::ETickGroup::TG_Update;
}

void FArcadeMovementComponent::Update(float deltaTime)
{
	if (!GetOwner())
	{
		return;
	}

	Enigma::FVector pos = GetOwner()->GetTransform().GetPosition();
	pos.X += VelX * deltaTime;
	pos.Y += VelY * deltaTime;
	GetOwner()->GetTransform().SetPosition(pos);

	// Reset velocity each frame (input callbacks re-set it)
	VelX = 0.0f;
	VelY = 0.0f;
}
