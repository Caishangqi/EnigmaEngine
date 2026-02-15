// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file IntVector.h
/// @brief 3D integer vector type.

#include "Math/MathUtility.h"

#include <cstdint>

namespace Enigma
{

// Forward declaration to avoid full include for conversion methods.
struct FVector;

/// @brief 3D integer vector (X, Y, Z).
///
/// Used for voxel coordinates, grid indices, and discrete 3D math.
/// All arithmetic is constexpr. Conversion to/from FVector requires
/// Vector.h and lives in IntVector.cpp.
struct CORE_API FIntVector
{
	/// X component.
	int32_t X;

	/// Y component.
	int32_t Y;

	/// Z component.
	int32_t Z;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Initializes to (0, 0, 0).
	constexpr FIntVector()
		: X(0)
		, Y(0)
		, Z(0)
	{
	}

	/// @brief Construct from explicit X, Y, Z values.
	constexpr FIntVector(int32_t InX, int32_t InY, int32_t InZ)
		: X(InX)
		, Y(InY)
		, Z(InZ)
	{
	}

	/// @brief Construct with all components set to the same value.
	explicit constexpr FIntVector(int32_t InValue)
		: X(InValue)
		, Y(InValue)
		, Z(InValue)
	{
	}

	// -----------------------------------------------------------------
	// Arithmetic operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition.
	constexpr FIntVector operator+(const FIntVector& Other) const
	{
		return FIntVector(X + Other.X, Y + Other.Y, Z + Other.Z);
	}

	/// @brief Component-wise subtraction.
	constexpr FIntVector operator-(const FIntVector& Other) const
	{
		return FIntVector(X - Other.X, Y - Other.Y, Z - Other.Z);
	}

	/// @brief Component-wise multiplication.
	constexpr FIntVector operator*(const FIntVector& Other) const
	{
		return FIntVector(X * Other.X, Y * Other.Y, Z * Other.Z);
	}

	/// @brief Component-wise division.
	constexpr FIntVector operator/(const FIntVector& Other) const
	{
		return FIntVector(X / Other.X, Y / Other.Y, Z / Other.Z);
	}

	/// @brief Component-wise modulo.
	constexpr FIntVector operator%(const FIntVector& Other) const
	{
		return FIntVector(X % Other.X, Y % Other.Y, Z % Other.Z);
	}

	/// @brief Scalar multiplication.
	constexpr FIntVector operator*(int32_t Scalar) const
	{
		return FIntVector(X * Scalar, Y * Scalar, Z * Scalar);
	}

	/// @brief Scalar division.
	constexpr FIntVector operator/(int32_t Scalar) const
	{
		return FIntVector(X / Scalar, Y / Scalar, Z / Scalar);
	}

	/// @brief Unary negation.
	constexpr FIntVector operator-() const
	{
		return FIntVector(-X, -Y, -Z);
	}

	// -----------------------------------------------------------------
	// Compound assignment operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition assignment.
	constexpr FIntVector& operator+=(const FIntVector& Other)
	{
		X += Other.X;
		Y += Other.Y;
		Z += Other.Z;
		return *this;
	}

	/// @brief Component-wise subtraction assignment.
	constexpr FIntVector& operator-=(const FIntVector& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		Z -= Other.Z;
		return *this;
	}

	/// @brief Scalar multiplication assignment.
	constexpr FIntVector& operator*=(int32_t Scalar)
	{
		X *= Scalar;
		Y *= Scalar;
		Z *= Scalar;
		return *this;
	}

	/// @brief Scalar division assignment.
	constexpr FIntVector& operator/=(int32_t Scalar)
	{
		X /= Scalar;
		Y /= Scalar;
		Z /= Scalar;
		return *this;
	}

	// -----------------------------------------------------------------
	// Comparison operators
	// -----------------------------------------------------------------

	/// @brief Exact equality.
	constexpr bool operator==(const FIntVector& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z;
	}

	/// @brief Exact inequality.
	constexpr bool operator!=(const FIntVector& Other) const
	{
		return X != Other.X || Y != Other.Y || Z != Other.Z;
	}

	// -----------------------------------------------------------------
	// Utility
	// -----------------------------------------------------------------

	/// @brief Check if this vector is exactly zero.
	constexpr bool IsZero() const
	{
		return X == 0 && Y == 0 && Z == 0;
	}

	// -----------------------------------------------------------------
	// Direction constants (right-hand Y-up coordinate system)
	// -----------------------------------------------------------------

	/// @brief Up direction (0, 1, 0).
	static constexpr FIntVector Up() { return FIntVector(0, 1, 0); }

	/// @brief Down direction (0, -1, 0).
	static constexpr FIntVector Down() { return FIntVector(0, -1, 0); }

	/// @brief Forward direction (0, 0, -1).
	static constexpr FIntVector Forward() { return FIntVector(0, 0, -1); }

	/// @brief Backward direction (0, 0, 1).
	static constexpr FIntVector Backward() { return FIntVector(0, 0, 1); }

	/// @brief Right direction (1, 0, 0).
	static constexpr FIntVector Right() { return FIntVector(1, 0, 0); }

	/// @brief Left direction (-1, 0, 0).
	static constexpr FIntVector Left() { return FIntVector(-1, 0, 0); }

	// -----------------------------------------------------------------
	// Conversion (declared here, defined in IntVector.cpp)
	// -----------------------------------------------------------------

	/// @brief Convert to floating-point FVector.
	FVector ToFloat() const;

	/// @brief Convert from FVector using floor semantics (voxel convention).
	static FIntVector FromFloat(const FVector& V);

	// -----------------------------------------------------------------
	// Predefined constants (defined in IntVector.cpp)
	// -----------------------------------------------------------------

	/// Zero value (0, 0, 0).
	static const FIntVector ZeroValue;

	/// One value (1, 1, 1).
	static const FIntVector OneValue;
};

} // namespace Enigma
