// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Transform.h
/// @brief Combined spatial transform (Translation + Rotation + Scale).

#include "Math/Quat.h"
#include "Math/Vector.h"

namespace Enigma
{

struct FMatrix;

/// @brief Combined transform: Translation, Rotation (FQuat), Scale (FVector).
///
/// Member order: Rotation, Translation, Scale3D (matches Unreal Engine).
/// ToMatrix composes Scale -> Rotation -> Translation.
/// TransformPosition applies full TRS; TransformVector applies Scale+Rotation only.
/// @note Non-uniform scale inverse and combine are approximate (standard game engine convention).
struct CORE_API FTransform
{
	/// Rotation component.
	FQuat Rotation;

	/// Translation component.
	FVector Translation;

	/// 3D scale component.
	FVector Scale3D;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Identity transform.
	constexpr FTransform()
		: Rotation()
		, Translation(0.0f, 0.0f, 0.0f)
		, Scale3D(1.0f, 1.0f, 1.0f)
	{
	}

	/// @brief Construct from translation only (identity rotation, unit scale).
	explicit constexpr FTransform(const FVector& InTranslation)
		: Rotation()
		, Translation(InTranslation)
		, Scale3D(1.0f, 1.0f, 1.0f)
	{
	}

	/// @brief Construct from rotation, translation, and optional scale.
	constexpr FTransform(const FQuat& InRotation, const FVector& InTranslation,
		const FVector& InScale3D = FVector(1.0f, 1.0f, 1.0f))
		: Rotation(InRotation)
		, Translation(InTranslation)
		, Scale3D(InScale3D)
	{
	}

	// -----------------------------------------------------------------
	// Operators
	// -----------------------------------------------------------------

	/// @brief Combine transforms (apply this first, then Other).
	FTransform operator*(const FTransform& Other) const;

	// -----------------------------------------------------------------
	// Transform operations
	// -----------------------------------------------------------------

	/// @brief Transform a position (Scale -> Rotation -> Translation).
	FVector TransformPosition(const FVector& V) const;

	/// @brief Transform a direction vector (Scale -> Rotation, no translation).
	FVector TransformVector(const FVector& V) const;

	/// @brief Build the 4x4 matrix: Scale * Rotation * Translation.
	FMatrix ToMatrix() const;

	/// @brief Compute the inverse transform.
	/// @note Approximate for non-uniform scale.
	FTransform GetInverse() const;

	// -----------------------------------------------------------------
	// Getters / Setters
	// -----------------------------------------------------------------

	/// @brief Get the translation component.
	constexpr FVector GetTranslation() const { return Translation; }

	/// @brief Get the rotation component.
	constexpr FQuat GetRotation() const { return Rotation; }

	/// @brief Get the scale component.
	constexpr FVector GetScale3D() const { return Scale3D; }

	/// @brief Set the translation component.
	constexpr void SetTranslation(const FVector& V) { Translation = V; }

	/// @brief Set the rotation component.
	constexpr void SetRotation(const FQuat& Q) { Rotation = Q; }

	/// @brief Set the scale component.
	constexpr void SetScale3D(const FVector& S) { Scale3D = S; }

	// -----------------------------------------------------------------
	// Comparison
	// -----------------------------------------------------------------

	/// @brief Check if two transforms are nearly equal within Tolerance.
	bool Equals(const FTransform& Other, float Tolerance = FMath::KindaSmallNumber) const;

	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	/// @brief Identity transform (no translation, no rotation, unit scale).
	static const FTransform Identity;
};

} // namespace Enigma
