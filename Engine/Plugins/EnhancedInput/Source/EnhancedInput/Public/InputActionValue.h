// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file InputActionValue.h
/// @brief Type-safe input value container, internally stored as FVector.

#include "InputKeys.h"  // for ENHANCEDINPUT_API
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Math/MathUtility.h"

#include <cstdint>

namespace Enigma
{

/// @brief Dimensionality of an input action's value.
enum class EInputActionValueType : uint8_t
{
	Boolean,
	Axis1D,
	Axis2D,
	Axis3D
};

/// @brief Type-safe input value. Internally stored as FVector.
///
/// UE equivalent: FInputActionValue (EnhancedInput/Public/InputActionValue.h)
struct ENHANCEDINPUT_API FInputActionValue
{
	FInputActionValue();
	explicit FInputActionValue(bool value);
	explicit FInputActionValue(float value);
	explicit FInputActionValue(FVector2D value);
	explicit FInputActionValue(FVector value);
	FInputActionValue(EInputActionValueType type, FVector value);

	/// Get value as specific type.
	template <typename T> T Get() const;

	EInputActionValueType GetValueType() const;
	bool IsNonZero(float tolerance = FMath::KindaSmallNumber) const;
	void Reset();

	FInputActionValue& operator+=(const FInputActionValue& rhs);
	FInputActionValue& operator*=(float scalar);

private:
	FVector Value;
	EInputActionValueType ValueType = EInputActionValueType::Boolean;
};

// Template specializations (declared here, defined in .cpp)
template <> ENHANCEDINPUT_API bool FInputActionValue::Get<bool>() const;
template <> ENHANCEDINPUT_API float FInputActionValue::Get<float>() const;
template <> ENHANCEDINPUT_API FVector2D FInputActionValue::Get<FVector2D>() const;
template <> ENHANCEDINPUT_API FVector FInputActionValue::Get<FVector>() const;

} // namespace Enigma
