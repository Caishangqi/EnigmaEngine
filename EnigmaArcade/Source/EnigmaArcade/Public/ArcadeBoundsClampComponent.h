// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "EnigmaArcadeAPI.generated.h"
#include "GameFramework/Component.h"
#include "Misc/Name.h"

// -------------------------------------------------------------
// FArcadeBoundsClampComponent
//
// Clamps the owner's Transform position to the framebuffer
// bounds each frame. Runs in TG_PostUpdate to ensure movement
// has already been applied.
//
// bCanEverTick = true, TickGroup = TG_PostUpdate
// -------------------------------------------------------------
class ENIGMAARCADE_API FArcadeBoundsClampComponent : public Enigma::FComponent
{
public:
	static Enigma::FName GetStaticName() { return Enigma::FName("ArcadeBoundsClamp"); }
	Enigma::FName GetName() const override { return GetStaticName(); }

	FArcadeBoundsClampComponent();

	void Update(float deltaTime) override;
};
