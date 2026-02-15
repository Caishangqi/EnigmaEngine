// Copyright EnigmaEngine. All Rights Reserved.

/// @file Rotator.cpp
/// @brief Implementation of FRotator non-constexpr functions.

#include "Math/Rotator.h"
#include "Math/Quat.h"
#include "Math/Matrix.h"

namespace Enigma
{

// -----------------------------------------------------------------
// Constants
// -----------------------------------------------------------------

const FRotator FRotator::ZeroRotator(0.0f, 0.0f, 0.0f);

// -----------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------

FRotator::FRotator(const FQuat& Quat)
{
	*this = Quat.ToRotator();
}

// -----------------------------------------------------------------
// Operations
// -----------------------------------------------------------------

FRotator FRotator::GetNormalized() const
{
	return FRotator(
		FMath::NormalizeAngle(Pitch),
		FMath::NormalizeAngle(Yaw),
		FMath::NormalizeAngle(Roll));
}

FQuat FRotator::Quaternion() const
{
	// Convert degrees to half-angle radians.
	const float HP = FMath::DegreesToRadians(Pitch) * 0.5f;
	const float HY = FMath::DegreesToRadians(Yaw) * 0.5f;
	const float HR = FMath::DegreesToRadians(Roll) * 0.5f;

	const float SP = FMath::Sin(HP);
	const float CP = FMath::Cos(HP);
	const float SY = FMath::Sin(HY);
	const float CY = FMath::Cos(HY);
	const float SR = FMath::Sin(HR);
	const float CR = FMath::Cos(HR);

	// YXZ intrinsic order: q = qY(yaw) * qX(pitch) * qZ(roll)
	return FQuat(
		CY * SP * CR + SY * CP * SR,
		SY * CP * CR - CY * SP * SR,
		CY * CP * SR - SY * SP * CR,
		CY * CP * CR + SY * SP * SR);
}

FMatrix FRotator::ToMatrix() const
{
	return Quaternion().ToMatrix();
}

} // namespace Enigma
