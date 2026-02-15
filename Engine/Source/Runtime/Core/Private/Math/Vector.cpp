// Copyright EnigmaEngine. All Rights Reserved.

/// @file Vector.cpp
/// @brief Implementation of FVector non-constexpr functions and static constants.

#include "Math/Vector.h"

namespace Enigma
{

// -----------------------------------------------------------------
// Non-constexpr operations
// -----------------------------------------------------------------

float FVector::Size() const
{
	return FMath::Sqrt(X * X + Y * Y + Z * Z);
}

FVector FVector::GetNormalized(float Tolerance) const
{
	const float SquaredSum = SizeSquared();
	if (SquaredSum > Tolerance)
	{
		const float Scale = FMath::InvSqrt(SquaredSum);
		return FVector(X * Scale, Y * Scale, Z * Scale);
	}
	return FVector(0.0f, 0.0f, 0.0f);
}

bool FVector::Normalize(float Tolerance)
{
	const float SquaredSum = SizeSquared();
	if (SquaredSum > Tolerance)
	{
		const float Scale = FMath::InvSqrt(SquaredSum);
		X *= Scale;
		Y *= Scale;
		Z *= Scale;
		return true;
	}
	return false;
}

float FVector::Distance(const FVector& A, const FVector& B)
{
	return FMath::Sqrt(DistSquared(A, B));
}

// -----------------------------------------------------------------
// Predefined constants (right-hand Y-up coordinate system)
// -----------------------------------------------------------------

const FVector FVector::ZeroVector     = FVector(0.0f, 0.0f, 0.0f);
const FVector FVector::OneVector      = FVector(1.0f, 1.0f, 1.0f);
const FVector FVector::UpVector       = FVector(0.0f, 1.0f, 0.0f);
const FVector FVector::DownVector     = FVector(0.0f, -1.0f, 0.0f);
const FVector FVector::ForwardVector  = FVector(0.0f, 0.0f, -1.0f);
const FVector FVector::BackwardVector = FVector(0.0f, 0.0f, 1.0f);
const FVector FVector::RightVector    = FVector(1.0f, 0.0f, 0.0f);
const FVector FVector::LeftVector     = FVector(-1.0f, 0.0f, 0.0f);

} // namespace Enigma
