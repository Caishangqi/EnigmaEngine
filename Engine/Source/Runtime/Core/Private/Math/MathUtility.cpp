// Copyright EnigmaEngine. All Rights Reserved.

/// @file MathUtility.cpp
/// @brief Implementation of FMath non-constexpr functions.

#include "Math/MathUtility.h"

namespace Enigma
{

// -----------------------------------------------------------------
// Trigonometry
// -----------------------------------------------------------------

float FMath::Sin(float Value)
{
	return std::sin(Value);
}

float FMath::Cos(float Value)
{
	return std::cos(Value);
}

float FMath::Tan(float Value)
{
	return std::tan(Value);
}

float FMath::Asin(float Value)
{
	return std::asin(Value);
}

float FMath::Acos(float Value)
{
	return std::acos(Value);
}

float FMath::Atan2(float Y, float X)
{
	return std::atan2(Y, X);
}

// -----------------------------------------------------------------
// Common math
// -----------------------------------------------------------------

float FMath::Sqrt(float Value)
{
	return std::sqrt(Value);
}

float FMath::InvSqrt(float Value)
{
	return 1.0f / std::sqrt(Value);
}

float FMath::Floor(float Value)
{
	return std::floor(Value);
}

float FMath::Ceil(float Value)
{
	return std::ceil(Value);
}

float FMath::Fmod(float X, float Y)
{
	return std::fmod(X, Y);
}

float FMath::Pow(float Base, float Exp)
{
	return std::pow(Base, Exp);
}

// -----------------------------------------------------------------
// Angle utilities
// -----------------------------------------------------------------

float FMath::NormalizeAngle(float Angle)
{
	// Normalize to [0, 360) first, then shift to [-180, 180)
	Angle = Fmod(Angle, 360.0f);
	if (Angle < 0.0f)
	{
		Angle += 360.0f;
	}
	if (Angle >= 180.0f)
	{
		Angle -= 360.0f;
	}
	return Angle;
}

float FMath::ClampAngle(float Angle)
{
	// Normalize to [0, 360)
	Angle = Fmod(Angle, 360.0f);
	if (Angle < 0.0f)
	{
		Angle += 360.0f;
	}
	return Angle;
}

} // namespace Enigma
