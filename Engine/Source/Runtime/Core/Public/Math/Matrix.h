// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Matrix.h
/// @brief 4x4 floating-point matrix type.

#include "Math/MathUtility.h"
#include "Math/Vector.h"

namespace Enigma
{

struct FQuat;

/// @brief 4x4 row-major floating-point matrix.
///
/// Storage: M[RowIndex][ColumnIndex], row-vector convention (v * M).
/// Translation is stored in Row 3: M[3][0], M[3][1], M[3][2].
/// Matrix multiply semantics: C = A * B applies A first, then B.
struct CORE_API FMatrix
{
	/// Raw 4x4 element storage (row-major).
	float M[4][4];

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Leaves elements uninitialized.
	FMatrix();

	/// @brief Construct from 16 explicit values (row-major order).
	constexpr FMatrix(
		float M00, float M01, float M02, float M03,
		float M10, float M11, float M12, float M13,
		float M20, float M21, float M22, float M23,
		float M30, float M31, float M32, float M33)
		: M{
			{M00, M01, M02, M03},
			{M10, M11, M12, M13},
			{M20, M21, M22, M23},
			{M30, M31, M32, M33}}
	{
	}

	// -----------------------------------------------------------------
	// Operators
	// -----------------------------------------------------------------

	/// @brief Matrix multiplication (C = A * B: apply A first, then B).
	FMatrix operator*(const FMatrix& Other) const;

	// -----------------------------------------------------------------
	// Transform operations
	// -----------------------------------------------------------------

	/// @brief Transform a position (w=1, includes translation).
	FVector TransformPosition(const FVector& V) const;

	/// @brief Transform a direction vector (w=0, no translation).
	FVector TransformVector(const FVector& V) const;

	// -----------------------------------------------------------------
	// Operations
	// -----------------------------------------------------------------

	/// @brief Return the transpose of this matrix.
	FMatrix GetTransposed() const;

	/// @brief Return the inverse via adjugate method.
	/// @note Returns Identity if the matrix is singular (|det| < SmallNumber).
	FMatrix GetInverse() const;

	/// @brief Compute the 4x4 determinant.
	float Determinant() const;

	/// @brief Check if two matrices are nearly equal within Tolerance.
	bool Equals(const FMatrix& Other, float Tolerance = FMath::KindaSmallNumber) const;

	// -----------------------------------------------------------------
	// Factory methods
	// -----------------------------------------------------------------

	/// @brief Build a translation matrix (translation in Row 3).
	static FMatrix MakeTranslation(const FVector& Translation);

	/// @brief Build a rotation matrix from a quaternion.
	static FMatrix MakeRotation(const FQuat& Rotation);

	/// @brief Build a non-uniform scale matrix.
	static FMatrix MakeScale(const FVector& Scale);

	/// @brief Build a uniform scale matrix.
	static FMatrix MakeScale(float UniformScale);

	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	/// @brief 4x4 identity matrix.
	static const FMatrix Identity;
};

} // namespace Enigma
