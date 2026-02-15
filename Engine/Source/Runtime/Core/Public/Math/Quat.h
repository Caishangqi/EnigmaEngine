// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Quat.h
/// @brief Quaternion rotation type.

#include "Math/MathUtility.h"
#include "Math/Vector.h"

namespace Enigma
{

struct FMatrix;
struct FRotator;

/// @brief Quaternion (X, Y, Z, W) for gimbal-lock-free rotation.
///
/// Identity is (0, 0, 0, 1). Storage order matches Unreal Engine.
/// Most operations assume a unit quaternion; the caller is responsible
/// for keeping quaternions normalized (use Normalize/GetNormalized).
struct CORE_API FQuat
{
	/// X component (vector part).
	float X;

	/// Y component (vector part).
	float Y;

	/// Z component (vector part).
	float Z;

	/// W component (scalar part).
	float W;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Initializes to identity (0,0,0,1).
	constexpr FQuat()
		: X(0.0f)
		, Y(0.0f)
		, Z(0.0f)
		, W(1.0f)
	{
	}

	/// @brief Construct from explicit components.
	constexpr FQuat(float InX, float InY, float InZ, float InW)
		: X(InX)
		, Y(InY)
		, Z(InZ)
		, W(InW)
	{
	}

	/// @brief Construct from axis and angle (radians).
	/// @param Axis Rotation axis (should be unit length).
	/// @param AngleRad Rotation angle in radians.
	FQuat(const FVector& Axis, float AngleRad);

	// -----------------------------------------------------------------
	// Operators
	// -----------------------------------------------------------------

	/// @brief Quaternion multiplication (rotation composition).
	FQuat operator*(const FQuat& Other) const;

	/// @brief Rotate a vector (shorthand for RotateVector).
	FVector operator*(const FVector& V) const;

	// -----------------------------------------------------------------
	// Operations
	// -----------------------------------------------------------------

	/// @brief Rotate a vector by this quaternion (q*v*q^-1).
	/// @note Assumes this quaternion is unit length.
	FVector RotateVector(const FVector& V) const;

	/// @brief Inverse-rotate a vector (q^-1*v*q).
	/// @note Assumes this quaternion is unit length.
	FVector UnrotateVector(const FVector& V) const;

	/// @brief Return a normalized copy.
	FQuat GetNormalized() const;

	/// @brief Normalize in-place. Returns true if successful.
	bool Normalize(float Tolerance = FMath::SmallNumber);

	/// @brief Quaternion norm (should be 1.0 for unit quaternions).
	float Size() const;

	/// @brief Squared norm (avoids sqrt).
	constexpr float SizeSquared() const
	{
		return X * X + Y * Y + Z * Z + W * W;
	}

	/// @brief Return the inverse (conjugate / |q|^2).
	FQuat GetInverse() const;

	/// @brief Convert to a 4x4 row-major rotation matrix.
	FMatrix ToMatrix() const;

	/// @brief Convert to Euler angles (Pitch, Yaw, Roll in degrees).
	FRotator ToRotator() const;

	/// @brief Check if two quaternions are nearly equal.
	constexpr bool Equals(const FQuat& Other, float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(X - Other.X) <= Tolerance
			&& FMath::Abs(Y - Other.Y) <= Tolerance
			&& FMath::Abs(Z - Other.Z) <= Tolerance
			&& FMath::Abs(W - Other.W) <= Tolerance;
	}

	// -----------------------------------------------------------------
	// Interpolation
	// -----------------------------------------------------------------

	/// @brief Spherical interpolation. Falls back to Nlerp when nearly parallel.
	static FQuat Slerp(const FQuat& A, const FQuat& B, float Alpha);

	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	/// @brief Identity quaternion (0, 0, 0, 1).
	static const FQuat Identity;
};

} // namespace Enigma
