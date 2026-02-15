// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Rotator.h
/// @brief Euler angle rotation type (Pitch, Yaw, Roll in degrees).

#include "Math/MathUtility.h"

namespace Enigma
{

struct FQuat;
struct FMatrix;

/// @brief Euler angle rotation in degrees.
///
/// Pitch = rotation around Right axis (X).
/// Yaw   = rotation around Up axis (Y).
/// Roll  = rotation around Forward axis (Z).
/// Intrinsic rotation order: YXZ (Yaw, then Pitch, then Roll).
struct CORE_API FRotator
{
	/// Rotation around the Right axis (X), in degrees.
	float Pitch;

	/// Rotation around the Up axis (Y), in degrees.
	float Yaw;

	/// Rotation around the Forward axis (Z), in degrees.
	float Roll;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Initializes to (0, 0, 0).
	constexpr FRotator()
		: Pitch(0.0f)
		, Yaw(0.0f)
		, Roll(0.0f)
	{
	}

	/// @brief Construct from explicit Pitch, Yaw, Roll (degrees).
	constexpr FRotator(float InPitch, float InYaw, float InRoll)
		: Pitch(InPitch)
		, Yaw(InYaw)
		, Roll(InRoll)
	{
	}

	/// @brief Construct from a quaternion (extracts Euler angles).
	explicit FRotator(const FQuat& Quat);

	// -----------------------------------------------------------------
	// Arithmetic operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition.
	constexpr FRotator operator+(const FRotator& Other) const
	{
		return FRotator(Pitch + Other.Pitch, Yaw + Other.Yaw, Roll + Other.Roll);
	}

	/// @brief Component-wise subtraction.
	constexpr FRotator operator-(const FRotator& Other) const
	{
		return FRotator(Pitch - Other.Pitch, Yaw - Other.Yaw, Roll - Other.Roll);
	}

	/// @brief Scalar multiplication.
	constexpr FRotator operator*(float Scalar) const
	{
		return FRotator(Pitch * Scalar, Yaw * Scalar, Roll * Scalar);
	}

	/// @brief Unary negation.
	constexpr FRotator operator-() const
	{
		return FRotator(-Pitch, -Yaw, -Roll);
	}

	// -----------------------------------------------------------------
	// Compound assignment operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition assignment.
	constexpr FRotator& operator+=(const FRotator& Other)
	{
		Pitch += Other.Pitch;
		Yaw += Other.Yaw;
		Roll += Other.Roll;
		return *this;
	}

	/// @brief Component-wise subtraction assignment.
	constexpr FRotator& operator-=(const FRotator& Other)
	{
		Pitch -= Other.Pitch;
		Yaw -= Other.Yaw;
		Roll -= Other.Roll;
		return *this;
	}

	/// @brief Scalar multiplication assignment.
	constexpr FRotator& operator*=(float Scalar)
	{
		Pitch *= Scalar;
		Yaw *= Scalar;
		Roll *= Scalar;
		return *this;
	}

	// -----------------------------------------------------------------
	// Comparison operators
	// -----------------------------------------------------------------

	/// @brief Exact equality.
	constexpr bool operator==(const FRotator& Other) const
	{
		return Pitch == Other.Pitch && Yaw == Other.Yaw && Roll == Other.Roll;
	}

	/// @brief Exact inequality.
	constexpr bool operator!=(const FRotator& Other) const
	{
		return Pitch != Other.Pitch || Yaw != Other.Yaw || Roll != Other.Roll;
	}

	// -----------------------------------------------------------------
	// Operations
	// -----------------------------------------------------------------

	/// @brief Return a copy with angles normalized to [-180, 180).
	FRotator GetNormalized() const;

	/// @brief Convert to a quaternion (YXZ intrinsic rotation order).
	FQuat Quaternion() const;

	/// @brief Convert to a 4x4 rotation matrix.
	FMatrix ToMatrix() const;

	// -----------------------------------------------------------------
	// Tolerance-based comparison
	// -----------------------------------------------------------------

	/// @brief Check if two rotators are nearly equal within Tolerance.
	constexpr bool Equals(const FRotator& Other, float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(Pitch - Other.Pitch) <= Tolerance
			&& FMath::Abs(Yaw - Other.Yaw) <= Tolerance
			&& FMath::Abs(Roll - Other.Roll) <= Tolerance;
	}

	/// @brief Check if this rotator is nearly zero within Tolerance.
	constexpr bool IsNearlyZero(float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(Pitch) <= Tolerance
			&& FMath::Abs(Yaw) <= Tolerance
			&& FMath::Abs(Roll) <= Tolerance;
	}

	/// @brief Check if this rotator is exactly zero.
	constexpr bool IsZero() const
	{
		return Pitch == 0.0f && Yaw == 0.0f && Roll == 0.0f;
	}

	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	/// @brief Zero rotator (0, 0, 0).
	static const FRotator ZeroRotator;
};

} // namespace Enigma
