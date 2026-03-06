// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputModifiers.h
/// @brief Input value modifier interface and built-in modifiers.

#include "InputKeys.h"  // for ENHANCEDINPUT_API
#include "InputActionValue.h"
#include "Math/Vector.h"

#include <cstdint>

namespace Enigma
{

/// @brief Abstract modifier interface. Transforms input values in the pipeline.
class ENHANCEDINPUT_API IInputModifier
{
public:
	virtual ~IInputModifier() = default;

	/// Apply modification to the input value.
	virtual FInputActionValue Modify(
		const FInputActionValue& currentValue,
		float deltaTime) const = 0;
};

// --- Built-in Modifiers ---

/// @brief Negates value components selectively.
class ENHANCEDINPUT_API FInputModifierNegate final : public IInputModifier
{
public:
	bool bX = true, bY = true, bZ = true;
	FInputActionValue Modify(const FInputActionValue& value, float dt) const override;
};

/// @brief Multiplies value by a configurable scale vector.
class ENHANCEDINPUT_API FInputModifierScalar final : public IInputModifier
{
public:
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
	FInputActionValue Modify(const FInputActionValue& value, float dt) const override;
};

/// @brief Multiplies value by delta time.
class ENHANCEDINPUT_API FInputModifierScaleByDeltaTime final : public IInputModifier
{
public:
	FInputActionValue Modify(const FInputActionValue& value, float dt) const override;
};

/// @brief Dead zone type for FInputModifierDeadZone.
enum class EDeadZoneType : uint8_t { Axial, Radial };

/// @brief Clamps values below threshold to zero.
class ENHANCEDINPUT_API FInputModifierDeadZone final : public IInputModifier
{
public:
	float LowerThreshold = 0.2f;
	float UpperThreshold = 1.0f;
	EDeadZoneType Type = EDeadZoneType::Axial;
	FInputActionValue Modify(const FInputActionValue& value, float dt) const override;
};

/// @brief Axis swizzle order.
enum class ESwizzleAxis : uint8_t { YXZ, ZYX, XZY, YZX, ZXY };

/// @brief Remaps axis components.
class ENHANCEDINPUT_API FInputModifierSwizzleAxis final : public IInputModifier
{
public:
	ESwizzleAxis Order = ESwizzleAxis::YXZ;
	FInputActionValue Modify(const FInputActionValue& value, float dt) const override;
};

} // namespace Enigma
