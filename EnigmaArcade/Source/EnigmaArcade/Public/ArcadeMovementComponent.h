// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EnigmaArcadeAPI.generated.h"
#include "GameFramework/Component.h"
#include "Misc/Name.h"

// -------------------------------------------------------------
// FArcadeMovementComponent
//
// Applies velocity to the owner's Transform each frame, then
// resets velocity to zero. Input callbacks set VelX/VelY.
//
// bCanEverTick = true, TickGroup = TG_Update
// -------------------------------------------------------------
class ENIGMAARCADE_API FArcadeMovementComponent : public Enigma::FComponent
{
public:
	static Enigma::FName GetStaticName() { return Enigma::FName("ArcadeMovement"); }
	Enigma::FName GetName() const override { return GetStaticName(); }

	FArcadeMovementComponent();

	void Update(float deltaTime) override;

	// Input-driven velocity (set by input callbacks on GameInstance)
	float VelX = 0.0f;
	float VelY = 0.0f;
	float MoveSpeed = 15.0f;
};
