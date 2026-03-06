// Copyright EnigmaEngine. All Rights Reserved.

/// @file InputActionValue.cpp
/// @brief Implementation of FInputActionValue.

#include "InputActionValue.h"

namespace Enigma
{

FInputActionValue::FInputActionValue()
	: Value(0.0f, 0.0f, 0.0f)
	, ValueType(EInputActionValueType::Boolean)
{
}

FInputActionValue::FInputActionValue(bool value)
	: Value(value ? 1.0f : 0.0f, 0.0f, 0.0f)
	, ValueType(EInputActionValueType::Boolean)
{
}

FInputActionValue::FInputActionValue(float value)
	: Value(value, 0.0f, 0.0f)
	, ValueType(EInputActionValueType::Axis1D)
{
}

FInputActionValue::FInputActionValue(FVector2D value)
	: Value(value.X, value.Y, 0.0f)
	, ValueType(EInputActionValueType::Axis2D)
{
}

FInputActionValue::FInputActionValue(FVector value)
	: Value(value)
	, ValueType(EInputActionValueType::Axis3D)
{
}

FInputActionValue::FInputActionValue(EInputActionValueType type, FVector value)
	: Value(value)
	, ValueType(type)
{
}

// Template specializations

template <>
bool FInputActionValue::Get<bool>() const
{
	return Value.X != 0.0f;
}

template <>
float FInputActionValue::Get<float>() const
{
	return Value.X;
}

template <>
FVector2D FInputActionValue::Get<FVector2D>() const
{
	return FVector2D(Value.X, Value.Y);
}

template <>
FVector FInputActionValue::Get<FVector>() const
{
	return Value;
}

EInputActionValueType FInputActionValue::GetValueType() const
{
	return ValueType;
}

bool FInputActionValue::IsNonZero(float tolerance) const
{
	switch (ValueType)
	{
	case EInputActionValueType::Boolean:
		return Value.X != 0.0f;
	case EInputActionValueType::Axis1D:
		return FMath::Abs(Value.X) > tolerance;
	case EInputActionValueType::Axis2D:
		return FMath::Abs(Value.X) > tolerance
			|| FMath::Abs(Value.Y) > tolerance;
	case EInputActionValueType::Axis3D:
		return FMath::Abs(Value.X) > tolerance
			|| FMath::Abs(Value.Y) > tolerance
			|| FMath::Abs(Value.Z) > tolerance;
	}
	return false;
}

void FInputActionValue::Reset()
{
	Value = FVector(0.0f, 0.0f, 0.0f);
}

FInputActionValue& FInputActionValue::operator+=(const FInputActionValue& rhs)
{
	Value = Value + rhs.Value;
	return *this;
}

FInputActionValue& FInputActionValue::operator*=(float scalar)
{
	Value = Value * scalar;
	return *this;
}

} // namespace Enigma
