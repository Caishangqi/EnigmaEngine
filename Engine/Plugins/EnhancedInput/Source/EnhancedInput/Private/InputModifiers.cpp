// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputModifiers.cpp
/// @brief Implementation of built-in input modifiers.

#include "InputModifiers.h"

namespace Enigma
{

FInputActionValue FInputModifierNegate::Modify(const FInputActionValue& value, float /*dt*/) const
{
	FVector v = value.Get<FVector>();
	return FInputActionValue(value.GetValueType(), FVector(
		bX ? -v.X : v.X,
		bY ? -v.Y : v.Y,
		bZ ? -v.Z : v.Z));
}

FInputActionValue FInputModifierScalar::Modify(const FInputActionValue& value, float /*dt*/) const
{
	FVector v = value.Get<FVector>();
	return FInputActionValue(value.GetValueType(), FVector(
		v.X * Scale.X,
		v.Y * Scale.Y,
		v.Z * Scale.Z));
}

FInputActionValue FInputModifierScaleByDeltaTime::Modify(const FInputActionValue& value, float dt) const
{
	FVector v = value.Get<FVector>();
	return FInputActionValue(value.GetValueType(), v * dt);
}

FInputActionValue FInputModifierDeadZone::Modify(const FInputActionValue& value, float /*dt*/) const
{
	FVector v = value.Get<FVector>();

	if (Type == EDeadZoneType::Axial)
	{
		// Per-component dead zone
		auto applyDZ = [&](float val) -> float
		{
			float absVal = FMath::Abs(val);
			if (absVal < LowerThreshold)
			{
				return 0.0f;
			}
			if (absVal > UpperThreshold)
			{
				return val > 0.0f ? 1.0f : -1.0f;
			}
			float sign = val > 0.0f ? 1.0f : -1.0f;
			return sign * (absVal - LowerThreshold) / (UpperThreshold - LowerThreshold);
		};

		return FInputActionValue(value.GetValueType(), FVector(
			applyDZ(v.X), applyDZ(v.Y), applyDZ(v.Z)));
	}
	else // Radial
	{
		float magnitude = FMath::Sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
		if (magnitude < LowerThreshold)
		{
			return FInputActionValue(value.GetValueType(), FVector(0.0f, 0.0f, 0.0f));
		}
		if (magnitude > UpperThreshold)
		{
			FVector normalized = v * (1.0f / magnitude);
			return FInputActionValue(value.GetValueType(), normalized);
		}
		float scaled = (magnitude - LowerThreshold) / (UpperThreshold - LowerThreshold);
		FVector normalized = v * (scaled / magnitude);
		return FInputActionValue(value.GetValueType(), normalized);
	}
}

FInputActionValue FInputModifierSwizzleAxis::Modify(const FInputActionValue& value, float /*dt*/) const
{
	FVector v = value.Get<FVector>();
	FVector result;

	switch (Order)
	{
	case ESwizzleAxis::YXZ: result = FVector(v.Y, v.X, v.Z); break;
	case ESwizzleAxis::ZYX: result = FVector(v.Z, v.Y, v.X); break;
	case ESwizzleAxis::XZY: result = FVector(v.X, v.Z, v.Y); break;
	case ESwizzleAxis::YZX: result = FVector(v.Y, v.Z, v.X); break;
	case ESwizzleAxis::ZXY: result = FVector(v.Z, v.X, v.Y); break;
	default:                result = v; break;
	}

	return FInputActionValue(value.GetValueType(), result);
}

} // namespace Enigma
