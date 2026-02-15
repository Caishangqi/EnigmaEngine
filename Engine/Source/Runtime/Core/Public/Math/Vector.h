// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Vector.h
/// @brief 3D floating-point vector type.

#include "Math/MathUtility.h"

namespace Enigma
{

/// @brief 3D floating-point vector (X, Y, Z).
///
/// Used for positions, directions, velocities, and general 3D math.
/// Pure arithmetic operations are constexpr; Size/Normalize delegate
/// to FMath::Sqrt and live in the .cpp file.
struct CORE_API FVector
{
	/// X component.
	float X;

	/// Y component.
	float Y;

	/// Z component.
	float Z;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Initializes to (0, 0, 0).
	constexpr FVector()
		: X(0.0f)
		, Y(0.0f)
		, Z(0.0f)
	{
	}

	/// @brief Construct from explicit X, Y, Z values.
	constexpr FVector(float InX, float InY, float InZ)
		: X(InX)
		, Y(InY)
		, Z(InZ)
	{
	}

	/// @brief Construct with all components set to the same value.
	explicit constexpr FVector(float InValue)
		: X(InValue)
		, Y(InValue)
		, Z(InValue)
	{
	}

	// -----------------------------------------------------------------
	// Arithmetic operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition.
	constexpr FVector operator+(const FVector& Other) const
	{
		return FVector(X + Other.X, Y + Other.Y, Z + Other.Z);
	}

	/// @brief Component-wise subtraction.
	constexpr FVector operator-(const FVector& Other) const
	{
		return FVector(X - Other.X, Y - Other.Y, Z - Other.Z);
	}

	/// @brief Component-wise multiplication.
	constexpr FVector operator*(const FVector& Other) const
	{
		return FVector(X * Other.X, Y * Other.Y, Z * Other.Z);
	}

	/// @brief Component-wise division.
	constexpr FVector operator/(const FVector& Other) const
	{
		return FVector(X / Other.X, Y / Other.Y, Z / Other.Z);
	}

	/// @brief Scalar multiplication.
	constexpr FVector operator*(float Scalar) const
	{
		return FVector(X * Scalar, Y * Scalar, Z * Scalar);
	}

	/// @brief Scalar division.
	constexpr FVector operator/(float Scalar) const
	{
		const float Inv = 1.0f / Scalar;
		return FVector(X * Inv, Y * Inv, Z * Inv);
	}

	/// @brief Unary negation.
	constexpr FVector operator-() const
	{
		return FVector(-X, -Y, -Z);
	}

	// -----------------------------------------------------------------
	// Compound assignment operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition assignment.
	constexpr FVector& operator+=(const FVector& Other)
	{
		X += Other.X;
		Y += Other.Y;
		Z += Other.Z;
		return *this;
	}

	/// @brief Component-wise subtraction assignment.
	constexpr FVector& operator-=(const FVector& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		Z -= Other.Z;
		return *this;
	}

	/// @brief Component-wise multiplication assignment.
	constexpr FVector& operator*=(const FVector& Other)
	{
		X *= Other.X;
		Y *= Other.Y;
		Z *= Other.Z;
		return *this;
	}

	/// @brief Scalar multiplication assignment.
	constexpr FVector& operator*=(float Scalar)
	{
		X *= Scalar;
		Y *= Scalar;
		Z *= Scalar;
		return *this;
	}

	/// @brief Scalar division assignment.
	constexpr FVector& operator/=(float Scalar)
	{
		const float Inv = 1.0f / Scalar;
		X *= Inv;
		Y *= Inv;
		Z *= Inv;
		return *this;
	}

	// -----------------------------------------------------------------
	// Comparison operators
	// -----------------------------------------------------------------

	/// @brief Exact equality (use Equals() for tolerance-based comparison).
	constexpr bool operator==(const FVector& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z;
	}

	/// @brief Exact inequality.
	constexpr bool operator!=(const FVector& Other) const
	{
		return X != Other.X || Y != Other.Y || Z != Other.Z;
	}

	// -----------------------------------------------------------------
	// Inline constexpr operations
	// -----------------------------------------------------------------

	/// @brief Squared length of the vector (avoids sqrt).
	constexpr float SizeSquared() const
	{
		return X * X + Y * Y + Z * Z;
	}

	/// @brief Check if two vectors are nearly equal within Tolerance.
	constexpr bool Equals(const FVector& Other, float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(X - Other.X) <= Tolerance
			&& FMath::Abs(Y - Other.Y) <= Tolerance
			&& FMath::Abs(Z - Other.Z) <= Tolerance;
	}

	/// @brief Check if this vector is nearly zero within Tolerance.
	constexpr bool IsNearlyZero(float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(X) <= Tolerance
			&& FMath::Abs(Y) <= Tolerance
			&& FMath::Abs(Z) <= Tolerance;
	}

	/// @brief Check if this vector is exactly zero.
	constexpr bool IsZero() const
	{
		return X == 0.0f && Y == 0.0f && Z == 0.0f;
	}

	// -----------------------------------------------------------------
	// Static constexpr operations
	// -----------------------------------------------------------------

	/// @brief Dot product of two vectors.
	static constexpr float DotProduct(const FVector& A, const FVector& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
	}

	/// @brief Cross product of two vectors.
	static constexpr FVector CrossProduct(const FVector& A, const FVector& B)
	{
		return FVector(
			A.Y * B.Z - A.Z * B.Y,
			A.Z * B.X - A.X * B.Z,
			A.X * B.Y - A.Y * B.X
		);
	}

	/// @brief Squared distance between two points (avoids sqrt).
	static constexpr float DistSquared(const FVector& A, const FVector& B)
	{
		return FMath::Square(A.X - B.X) + FMath::Square(A.Y - B.Y) + FMath::Square(A.Z - B.Z);
	}

	// -----------------------------------------------------------------
	// Non-constexpr operations (declared here, defined in Vector.cpp)
	// -----------------------------------------------------------------

	/// @brief Length of the vector.
	float Size() const;

	/// @brief Return a normalized copy. Returns zero vector if too small.
	FVector GetNormalized(float Tolerance = FMath::SmallNumber) const;

	/// @brief Normalize this vector in-place. Returns true if successful.
	bool Normalize(float Tolerance = FMath::SmallNumber);

	/// @brief Euclidean distance between two points.
	static float Distance(const FVector& A, const FVector& B);

	// -----------------------------------------------------------------
	// Predefined constants (defined in Vector.cpp)
	// -----------------------------------------------------------------

	/// Zero vector (0, 0, 0).
	static const FVector ZeroVector;

	/// One vector (1, 1, 1).
	static const FVector OneVector;

	/// Up direction (0, 1, 0) -- Y-up coordinate system.
	static const FVector UpVector;

	/// Down direction (0, -1, 0).
	static const FVector DownVector;

	/// Forward direction (0, 0, -1) -- negative Z.
	static const FVector ForwardVector;

	/// Backward direction (0, 0, 1).
	static const FVector BackwardVector;

	/// Right direction (1, 0, 0).
	static const FVector RightVector;

	/// Left direction (-1, 0, 0).
	static const FVector LeftVector;
};

/// @brief Scalar * Vector (commutative support).
constexpr FVector operator*(float Scalar, const FVector& V)
{
	return FVector(Scalar * V.X, Scalar * V.Y, Scalar * V.Z);
}

} // namespace Enigma
