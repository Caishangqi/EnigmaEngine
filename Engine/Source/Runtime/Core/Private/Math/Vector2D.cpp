// Copyright EnigmaEngine. All Rights Reserved.

/// @file Vector2D.cpp
/// @brief Implementation of FVector2D non-constexpr functions.

#include "Math/Vector2D.h"

namespace Enigma
{

float FVector2D::Size() const
{
	return FMath::Sqrt(X * X + Y * Y);
}

FVector2D FVector2D::GetNormalized(float Tolerance) const
{
	const float SquaredSum = SizeSquared();
	if (SquaredSum > Tolerance)
	{
		const float Scale = FMath::InvSqrt(SquaredSum);
		return FVector2D(X * Scale, Y * Scale);
	}
	return FVector2D(0.0f, 0.0f);
}

bool FVector2D::Normalize(float Tolerance)
{
	const float SquaredSum = SizeSquared();
	if (SquaredSum > Tolerance)
	{
		const float Scale = FMath::InvSqrt(SquaredSum);
		X *= Scale;
		Y *= Scale;
		return true;
	}
	return false;
}

float FVector2D::Distance(const FVector2D& A, const FVector2D& B)
{
	return FMath::Sqrt(FMath::Square(A.X - B.X) + FMath::Square(A.Y - B.Y));
}

} // namespace Enigma
