// Copyright EnigmaEngine. All Rights Reserved.

/// @file Transform.cpp
/// @brief Implementation of FTransform non-constexpr functions.

#include "Math/Transform.h"
#include "Math/Matrix.h"

namespace Enigma
{

// -----------------------------------------------------------------
// Constants
// -----------------------------------------------------------------

const FTransform FTransform::Identity;

// -----------------------------------------------------------------
// Operators
// -----------------------------------------------------------------

FTransform FTransform::operator*(const FTransform& B) const
{
	// Combine: apply *this first, then B.
	// Result.Rotation = B.Rotation * A.Rotation
	// Result.Scale3D  = B.Scale3D * A.Scale3D  (component-wise)
	// Result.Translation = B.Rotation.Rotate(B.Scale3D * A.Translation) + B.Translation
	FTransform Result;
	Result.Rotation = B.Rotation * Rotation;
	Result.Scale3D = FVector(B.Scale3D.X * Scale3D.X,
		B.Scale3D.Y * Scale3D.Y,
		B.Scale3D.Z * Scale3D.Z);
	const FVector ScaledTranslation(
		B.Scale3D.X * Translation.X,
		B.Scale3D.Y * Translation.Y,
		B.Scale3D.Z * Translation.Z);
	Result.Translation = B.Rotation.RotateVector(ScaledTranslation) + B.Translation;
	return Result;
}

// -----------------------------------------------------------------
// Transform operations
// -----------------------------------------------------------------

FVector FTransform::TransformPosition(const FVector& V) const
{
	// Scale -> Rotate -> Translate
	const FVector Scaled(Scale3D.X * V.X, Scale3D.Y * V.Y, Scale3D.Z * V.Z);
	return Rotation.RotateVector(Scaled) + Translation;
}

FVector FTransform::TransformVector(const FVector& V) const
{
	// Scale -> Rotate (no translation)
	const FVector Scaled(Scale3D.X * V.X, Scale3D.Y * V.Y, Scale3D.Z * V.Z);
	return Rotation.RotateVector(Scaled);
}

FMatrix FTransform::ToMatrix() const
{
	// Compose: Scale * Rotation * Translation
	const FMatrix ScaleMat = FMatrix::MakeScale(Scale3D);
	const FMatrix RotMat = FMatrix::MakeRotation(Rotation);
	const FMatrix TransMat = FMatrix::MakeTranslation(Translation);
	return ScaleMat * RotMat * TransMat;
}

FTransform FTransform::GetInverse() const
{
	const FQuat InvRotation = Rotation.GetInverse();

	// Safe reciprocal scale (avoid division by zero).
	const FVector InvScale(
		FMath::Abs(Scale3D.X) > FMath::SmallNumber ? 1.0f / Scale3D.X : 0.0f,
		FMath::Abs(Scale3D.Y) > FMath::SmallNumber ? 1.0f / Scale3D.Y : 0.0f,
		FMath::Abs(Scale3D.Z) > FMath::SmallNumber ? 1.0f / Scale3D.Z : 0.0f);

	const FVector NegTranslation(-Translation.X, -Translation.Y, -Translation.Z);
	const FVector ScaledNegT(
		InvScale.X * NegTranslation.X,
		InvScale.Y * NegTranslation.Y,
		InvScale.Z * NegTranslation.Z);
	const FVector InvTranslation = InvRotation.RotateVector(ScaledNegT);

	return FTransform(InvRotation, InvTranslation, InvScale);
}

// -----------------------------------------------------------------
// Comparison
// -----------------------------------------------------------------

bool FTransform::Equals(const FTransform& Other, float Tolerance) const
{
	return Rotation.Equals(Other.Rotation, Tolerance)
		&& Translation.Equals(Other.Translation, Tolerance)
		&& Scale3D.Equals(Other.Scale3D, Tolerance);
}

} // namespace Enigma
