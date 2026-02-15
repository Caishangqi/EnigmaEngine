// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Vector2D.h
/// @brief 2D floating-point vector type.

#include "Math/MathUtility.h"

namespace Enigma
{

/// @brief 2D floating-point vector (X, Y).
///
/// Used for UI coordinates, texture coordinates, and 2D math.
/// Pure arithmetic operations are constexpr; Size() delegates to
/// FMath::Sqrt and is therefore not constexpr.
struct CORE_API FVector2D
{
	/// X component.
	float X;

	/// Y component.
	float Y;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Initializes to (0, 0).
	constexpr FVector2D()
		: X(0.0f)
		, Y(0.0f)
	{
	}

	/// @brief Construct from explicit X, Y values.
	constexpr FVector2D(float InX, float InY)
		: X(InX)
		, Y(InY)
	{
	}

	/// @brief Construct with both components set to the same value.
	explicit constexpr FVector2D(float InValue)
		: X(InValue)
		, Y(InValue)
	{
	}

	// -----------------------------------------------------------------
	// Arithmetic operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition.
	constexpr FVector2D operator+(const FVector2D& Other) const
	{
		return FVector2D(X + Other.X, Y + Other.Y);
	}

	/// @brief Component-wise subtraction.
	constexpr FVector2D operator-(const FVector2D& Other) const
	{
		return FVector2D(X - Other.X, Y - Other.Y);
	}

	/// @brief Component-wise multiplication.
	constexpr FVector2D operator*(const FVector2D& Other) const
	{
		return FVector2D(X * Other.X, Y * Other.Y);
	}

	/// @brief Component-wise division.
	constexpr FVector2D operator/(const FVector2D& Other) const
	{
		return FVector2D(X / Other.X, Y / Other.Y);
	}

	/// @brief Scalar multiplication.
	constexpr FVector2D operator*(float Scalar) const
	{
		return FVector2D(X * Scalar, Y * Scalar);
	}

	/// @brief Scalar division.
	constexpr FVector2D operator/(float Scalar) const
	{
		const float Inv = 1.0f / Scalar;
		return FVector2D(X * Inv, Y * Inv);
	}

	/// @brief Unary negation.
	constexpr FVector2D operator-() const
	{
		return FVector2D(-X, -Y);
	}

	// -----------------------------------------------------------------
	// Compound assignment operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition assignment.
	constexpr FVector2D& operator+=(const FVector2D& Other)
	{
		X += Other.X;
		Y += Other.Y;
		return *this;
	}

	/// @brief Component-wise subtraction assignment.
	constexpr FVector2D& operator-=(const FVector2D& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		return *this;
	}

	/// @brief Component-wise multiplication assignment.
	constexpr FVector2D& operator*=(const FVector2D& Other)
	{
		X *= Other.X;
		Y *= Other.Y;
		return *this;
	}

	/// @brief Scalar multiplication assignment.
	constexpr FVector2D& operator*=(float Scalar)
	{
		X *= Scalar;
		Y *= Scalar;
		return *this;
	}

	/// @brief Scalar division assignment.
	constexpr FVector2D& operator/=(float Scalar)
	{
		const float Inv = 1.0f / Scalar;
		X *= Inv;
		Y *= Inv;
		return *this;
	}

	// -----------------------------------------------------------------
	// Comparison operators
	// -----------------------------------------------------------------

	/// @brief Exact equality (use Equals() for tolerance-based comparison).
	constexpr bool operator==(const FVector2D& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}

	/// @brief Exact inequality.
	constexpr bool operator!=(const FVector2D& Other) const
	{
		return X != Other.X || Y != Other.Y;
	}

	// -----------------------------------------------------------------
	// Operations
	// -----------------------------------------------------------------

	/// @brief Length of the vector.
	float Size() const;

	/// @brief Squared length of the vector (avoids sqrt).
	constexpr float SizeSquared() const
	{
		return X * X + Y * Y;
	}

	/// @brief Return a normalized copy. Returns zero vector if too small.
	FVector2D GetNormalized(float Tolerance = FMath::SmallNumber) const;

	/// @brief Normalize this vector in-place. Returns true if successful.
	bool Normalize(float Tolerance = FMath::SmallNumber);

	// -----------------------------------------------------------------
	// Tolerance-based comparison
	// -----------------------------------------------------------------

	/// @brief Check if two vectors are nearly equal within Tolerance.
	constexpr bool Equals(const FVector2D& Other, float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(X - Other.X) <= Tolerance
			&& FMath::Abs(Y - Other.Y) <= Tolerance;
	}

	/// @brief Check if this vector is nearly zero within Tolerance.
	constexpr bool IsNearlyZero(float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(X) <= Tolerance
			&& FMath::Abs(Y) <= Tolerance;
	}

	/// @brief Check if this vector is exactly zero.
	constexpr bool IsZero() const
	{
		return X == 0.0f && Y == 0.0f;
	}

	// -----------------------------------------------------------------
	// Static operations
	// -----------------------------------------------------------------

	/// @brief Dot product of two 2D vectors.
	static constexpr float DotProduct(const FVector2D& A, const FVector2D& B)
	{
		return A.X * B.X + A.Y * B.Y;
	}

	/// @brief Cross product magnitude (Z component of the 3D cross product).
	static constexpr float CrossProduct(const FVector2D& A, const FVector2D& B)
	{
		return A.X * B.Y - A.Y * B.X;
	}

	/// @brief Euclidean distance between two points.
	static float Distance(const FVector2D& A, const FVector2D& B);

	/// @brief Squared distance between two points (avoids sqrt).
	static constexpr float DistSquared(const FVector2D& A, const FVector2D& B)
	{
		return FMath::Square(A.X - B.X) + FMath::Square(A.Y - B.Y);
	}

	// -----------------------------------------------------------------
	// Predefined constants
	// -----------------------------------------------------------------

	/// @brief Zero vector (0, 0).
	static constexpr FVector2D Zero() { return FVector2D(0.0f, 0.0f); }

	/// @brief One vector (1, 1).
	static constexpr FVector2D One() { return FVector2D(1.0f, 1.0f); }

	/// @brief Unit X axis (1, 0).
	static constexpr FVector2D UnitX() { return FVector2D(1.0f, 0.0f); }

	/// @brief Unit Y axis (0, 1).
	static constexpr FVector2D UnitY() { return FVector2D(0.0f, 1.0f); }
};

/// @brief Scalar * Vector (commutative support).
constexpr FVector2D operator*(float Scalar, const FVector2D& V)
{
	return FVector2D(Scalar * V.X, Scalar * V.Y);
}

} // namespace Enigma
