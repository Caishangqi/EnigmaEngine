// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Vector4.h
/// @brief 4D floating-point vector type.

#include "Math/Vector.h"

namespace Enigma
{

/// @brief 4D floating-point vector (X, Y, Z, W).
///
/// Used for homogeneous coordinates and shader data.
/// Provides explicit construction from FVector with configurable W
/// (1.0 for points, 0.0 for directions). Pure arithmetic is constexpr;
/// Size() delegates to FMath::Sqrt.
struct CORE_API FVector4
{
	/// X component.
	float X;

	/// Y component.
	float Y;

	/// Z component.
	float Z;

	/// W component.
	float W;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Initializes to (0, 0, 0, 0).
	constexpr FVector4()
		: X(0.0f)
		, Y(0.0f)
		, Z(0.0f)
		, W(0.0f)
	{
	}

	/// @brief Construct from explicit X, Y, Z, W values.
	constexpr FVector4(float InX, float InY, float InZ, float InW)
		: X(InX)
		, Y(InY)
		, Z(InZ)
		, W(InW)
	{
	}

	/// @brief Construct from FVector with configurable W.
	/// @param V Source 3D vector.
	/// @param InW W component (1.0 for points, 0.0 for directions).
	explicit constexpr FVector4(const FVector& V, float InW = 1.0f)
		: X(V.X)
		, Y(V.Y)
		, Z(V.Z)
		, W(InW)
	{
	}

	// -----------------------------------------------------------------
	// Arithmetic operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition.
	constexpr FVector4 operator+(const FVector4& Other) const
	{
		return FVector4(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W);
	}

	/// @brief Component-wise subtraction.
	constexpr FVector4 operator-(const FVector4& Other) const
	{
		return FVector4(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W);
	}

	/// @brief Component-wise multiplication.
	constexpr FVector4 operator*(const FVector4& Other) const
	{
		return FVector4(X * Other.X, Y * Other.Y, Z * Other.Z, W * Other.W);
	}

	/// @brief Scalar multiplication.
	constexpr FVector4 operator*(float Scalar) const
	{
		return FVector4(X * Scalar, Y * Scalar, Z * Scalar, W * Scalar);
	}

	/// @brief Scalar division.
	constexpr FVector4 operator/(float Scalar) const
	{
		const float Inv = 1.0f / Scalar;
		return FVector4(X * Inv, Y * Inv, Z * Inv, W * Inv);
	}

	/// @brief Unary negation.
	constexpr FVector4 operator-() const
	{
		return FVector4(-X, -Y, -Z, -W);
	}

	// -----------------------------------------------------------------
	// Compound assignment operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition assignment.
	constexpr FVector4& operator+=(const FVector4& Other)
	{
		X += Other.X;
		Y += Other.Y;
		Z += Other.Z;
		W += Other.W;
		return *this;
	}

	/// @brief Component-wise subtraction assignment.
	constexpr FVector4& operator-=(const FVector4& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		Z -= Other.Z;
		W -= Other.W;
		return *this;
	}

	/// @brief Scalar multiplication assignment.
	constexpr FVector4& operator*=(float Scalar)
	{
		X *= Scalar;
		Y *= Scalar;
		Z *= Scalar;
		W *= Scalar;
		return *this;
	}

	/// @brief Scalar division assignment.
	constexpr FVector4& operator/=(float Scalar)
	{
		const float Inv = 1.0f / Scalar;
		X *= Inv;
		Y *= Inv;
		Z *= Inv;
		W *= Inv;
		return *this;
	}

	// -----------------------------------------------------------------
	// Comparison operators
	// -----------------------------------------------------------------

	/// @brief Exact equality.
	constexpr bool operator==(const FVector4& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z && W == Other.W;
	}

	/// @brief Exact inequality.
	constexpr bool operator!=(const FVector4& Other) const
	{
		return X != Other.X || Y != Other.Y || Z != Other.Z || W != Other.W;
	}

	// -----------------------------------------------------------------
	// Operations
	// -----------------------------------------------------------------

	/// @brief 4D length of the vector.
	float Size() const;

	/// @brief Squared 4D length (avoids sqrt).
	constexpr float SizeSquared() const
	{
		return X * X + Y * Y + Z * Z + W * W;
	}

	/// @brief 3D length (ignoring W).
	float Size3() const;

	/// @brief Squared 3D length (ignoring W, avoids sqrt).
	constexpr float SizeSquared3() const
	{
		return X * X + Y * Y + Z * Z;
	}

	/// @brief Check if two vectors are nearly equal within Tolerance.
	constexpr bool Equals(const FVector4& Other, float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(X - Other.X) <= Tolerance
			&& FMath::Abs(Y - Other.Y) <= Tolerance
			&& FMath::Abs(Z - Other.Z) <= Tolerance
			&& FMath::Abs(W - Other.W) <= Tolerance;
	}

	/// @brief Check if this vector is nearly zero within Tolerance.
	constexpr bool IsNearlyZero(float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(X) <= Tolerance
			&& FMath::Abs(Y) <= Tolerance
			&& FMath::Abs(Z) <= Tolerance
			&& FMath::Abs(W) <= Tolerance;
	}

	// -----------------------------------------------------------------
	// Static operations
	// -----------------------------------------------------------------

	/// @brief 4D dot product.
	static constexpr float DotProduct(const FVector4& A, const FVector4& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
	}

	/// @brief 3D dot product (ignoring W).
	static constexpr float DotProduct3(const FVector4& A, const FVector4& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
	}
};

/// @brief Scalar * Vector4 (commutative support).
constexpr FVector4 operator*(float Scalar, const FVector4& V)
{
	return FVector4(Scalar * V.X, Scalar * V.Y, Scalar * V.Z, Scalar * V.W);
}

} // namespace Enigma
