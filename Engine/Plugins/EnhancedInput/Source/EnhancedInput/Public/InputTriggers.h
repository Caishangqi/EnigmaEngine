// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputTriggers.h
/// @brief Input trigger interface, state machine, and built-in triggers.

#include "InputKeys.h"  // for ENHANCEDINPUT_API
#include "InputActionValue.h"

#include <cstdint>

namespace Enigma
{

/// @brief Trigger evaluation result (per-frame).
enum class ETriggerState : uint8_t
{
	None,       // Not actuated
	Ongoing,    // Conditions being monitored, not yet confirmed
	Triggered   // All conditions met
};

/// @brief Events emitted from trigger state transitions.
enum class ETriggerEvent : uint8_t
{
	None       = 0,
	Started    = 1 << 0,  // None -> Ongoing or None -> Triggered
	Ongoing    = 1 << 1,  // Ongoing -> Ongoing
	Triggered  = 1 << 2,  // -> Triggered (primary fire event)
	Completed  = 1 << 3,  // Triggered -> None
	Canceled   = 1 << 4   // Ongoing -> None
};

/// Bitwise OR for ETriggerEvent.
inline ETriggerEvent operator|(ETriggerEvent a, ETriggerEvent b)
{
	return static_cast<ETriggerEvent>(
		static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/// Bitwise AND for ETriggerEvent.
inline ETriggerEvent operator&(ETriggerEvent a, ETriggerEvent b)
{
	return static_cast<ETriggerEvent>(
		static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/// @brief Abstract trigger interface.
class ENHANCEDINPUT_API IInputTrigger
{
public:
	virtual ~IInputTrigger() = default;

	/// Evaluate trigger state for this frame.
	virtual ETriggerState UpdateState(
		const FInputActionValue& value,
		float deltaTime) = 0;

	/// Actuation threshold for digital interpretation of analog values.
	float ActuationThreshold = 0.5f;

protected:
	/// Helper: is the value above the actuation threshold?
	bool IsActuated(const FInputActionValue& value) const;
};

// --- Built-in Triggers ---

/// @brief Fires Triggered every frame while actuated (continuous).
class ENHANCEDINPUT_API FInputTriggerDown final : public IInputTrigger
{
public:
	ETriggerState UpdateState(const FInputActionValue& value, float dt) override;
};

/// @brief Fires Triggered once on the frame actuation begins (one-shot).
class ENHANCEDINPUT_API FInputTriggerPressed final : public IInputTrigger
{
public:
	ETriggerState UpdateState(const FInputActionValue& value, float dt) override;
private:
	bool bWasActuated = false;
};

/// @brief Fires Triggered once on the frame actuation ends.
class ENHANCEDINPUT_API FInputTriggerReleased final : public IInputTrigger
{
public:
	ETriggerState UpdateState(const FInputActionValue& value, float dt) override;
private:
	bool bWasActuated = false;
};

/// @brief Fires Triggered once after held for HoldTimeThreshold seconds.
class ENHANCEDINPUT_API FInputTriggerHold final : public IInputTrigger
{
public:
	float HoldTimeThreshold = 0.5f;
	ETriggerState UpdateState(const FInputActionValue& value, float dt) override;
private:
	float HeldDuration = 0.0f;
	bool bTriggered = false;
};

} // namespace Enigma
