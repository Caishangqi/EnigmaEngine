// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputTriggers.cpp
/// @brief Implementation of built-in input triggers.

#include "InputTriggers.h"

namespace Enigma
{

// -----------------------------------------------------------------
// IInputTrigger helpers
// -----------------------------------------------------------------

bool IInputTrigger::IsActuated(const FInputActionValue& value) const
{
	FVector v = value.Get<FVector>();
	float magnitude = FMath::Sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
	return magnitude >= ActuationThreshold;
}

// -----------------------------------------------------------------
// FInputTriggerDown -- Triggered every frame while actuated
// -----------------------------------------------------------------

ETriggerState FInputTriggerDown::UpdateState(const FInputActionValue& value, float /*dt*/)
{
	return IsActuated(value) ? ETriggerState::Triggered : ETriggerState::None;
}

// -----------------------------------------------------------------
// FInputTriggerPressed -- Triggered once on press
// -----------------------------------------------------------------

ETriggerState FInputTriggerPressed::UpdateState(const FInputActionValue& value, float /*dt*/)
{
	bool bIsActuated = IsActuated(value);
	ETriggerState state = ETriggerState::None;

	if (bIsActuated && !bWasActuated)
	{
		state = ETriggerState::Triggered;
	}
	else if (bIsActuated)
	{
		state = ETriggerState::None;
	}

	bWasActuated = bIsActuated;
	return state;
}

// -----------------------------------------------------------------
// FInputTriggerReleased -- Triggered once on release
// -----------------------------------------------------------------

ETriggerState FInputTriggerReleased::UpdateState(const FInputActionValue& value, float /*dt*/)
{
	bool bIsActuated = IsActuated(value);
	ETriggerState state = ETriggerState::None;

	if (!bIsActuated && bWasActuated)
	{
		state = ETriggerState::Triggered;
	}

	bWasActuated = bIsActuated;
	return state;
}

// -----------------------------------------------------------------
// FInputTriggerHold -- Triggered once after held for threshold
// -----------------------------------------------------------------

ETriggerState FInputTriggerHold::UpdateState(const FInputActionValue& value, float dt)
{
	if (!IsActuated(value))
	{
		HeldDuration = 0.0f;
		bTriggered = false;
		return ETriggerState::None;
	}

	HeldDuration += dt;

	if (!bTriggered && HeldDuration >= HoldTimeThreshold)
	{
		bTriggered = true;
		return ETriggerState::Triggered;
	}

	if (bTriggered)
	{
		return ETriggerState::None;
	}

	return ETriggerState::Ongoing;
}

} // namespace Enigma
