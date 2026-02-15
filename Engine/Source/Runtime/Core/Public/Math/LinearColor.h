// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file LinearColor.h
/// @brief Linear-space floating-point color (R, G, B, A).

#include "Math/MathUtility.h"

namespace Enigma
{

struct FColor;

/// @brief Linear-space floating-point color.
///
/// Components are in [0,1] range for standard colors but may exceed
/// that range for HDR. Alpha defaults to 1.0 (fully opaque).
/// Use ToFColor() to convert to 8-bit sRGB for display.
struct CORE_API FLinearColor
{
	/// Red component.
	float R;

	/// Green component.
	float G;

	/// Blue component.
	float B;

	/// Alpha component.
	float A;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Black transparent (0,0,0,0).
	constexpr FLinearColor()
		: R(0.0f), G(0.0f), B(0.0f), A(0.0f)
	{
	}

	/// @brief Construct from explicit components (A defaults to 1.0).
	constexpr FLinearColor(float InR, float InG, float InB, float InA = 1.0f)
		: R(InR), G(InG), B(InB), A(InA)
	{
	}

	/// @brief Construct from an 8-bit sRGB FColor (sRGB to linear).
	explicit FLinearColor(const FColor& Color);

	// -----------------------------------------------------------------
	// Arithmetic operators
	// -----------------------------------------------------------------

	/// @brief Component-wise addition.
	constexpr FLinearColor operator+(const FLinearColor& Other) const
	{
		return FLinearColor(R + Other.R, G + Other.G, B + Other.B, A + Other.A);
	}

	/// @brief Component-wise subtraction.
	constexpr FLinearColor operator-(const FLinearColor& Other) const
	{
		return FLinearColor(R - Other.R, G - Other.G, B - Other.B, A - Other.A);
	}

	/// @brief Component-wise multiplication (color blending).
	constexpr FLinearColor operator*(const FLinearColor& Other) const
	{
		return FLinearColor(R * Other.R, G * Other.G, B * Other.B, A * Other.A);
	}

	/// @brief Scalar multiplication.
	constexpr FLinearColor operator*(float Scalar) const
	{
		return FLinearColor(R * Scalar, G * Scalar, B * Scalar, A * Scalar);
	}

	/// @brief Scalar multiplication (commutative).
	friend constexpr FLinearColor operator*(float Scalar, const FLinearColor& Color)
	{
		return Color * Scalar;
	}

	// -----------------------------------------------------------------
	// Compound assignment
	// -----------------------------------------------------------------

	/// @brief Component-wise addition assignment.
	constexpr FLinearColor& operator+=(const FLinearColor& Other)
	{
		R += Other.R; G += Other.G; B += Other.B; A += Other.A;
		return *this;
	}

	/// @brief Component-wise subtraction assignment.
	constexpr FLinearColor& operator-=(const FLinearColor& Other)
	{
		R -= Other.R; G -= Other.G; B -= Other.B; A -= Other.A;
		return *this;
	}

	/// @brief Scalar multiplication assignment.
	constexpr FLinearColor& operator*=(float Scalar)
	{
		R *= Scalar; G *= Scalar; B *= Scalar; A *= Scalar;
		return *this;
	}

	// -----------------------------------------------------------------
	// Comparison
	// -----------------------------------------------------------------

	/// @brief Exact equality.
	constexpr bool operator==(const FLinearColor& Other) const
	{
		return R == Other.R && G == Other.G && B == Other.B && A == Other.A;
	}

	/// @brief Exact inequality.
	constexpr bool operator!=(const FLinearColor& Other) const
	{
		return !(*this == Other);
	}

	/// @brief Check if two colors are nearly equal within Tolerance.
	constexpr bool Equals(const FLinearColor& Other, float Tolerance = FMath::KindaSmallNumber) const
	{
		return FMath::Abs(R - Other.R) <= Tolerance
			&& FMath::Abs(G - Other.G) <= Tolerance
			&& FMath::Abs(B - Other.B) <= Tolerance
			&& FMath::Abs(A - Other.A) <= Tolerance;
	}

	// -----------------------------------------------------------------
	// Conversion
	// -----------------------------------------------------------------

	/// @brief Convert to 8-bit sRGB FColor.
	/// @param bSRGB If true, apply linear-to-sRGB transfer function.
	FColor ToFColor(bool bSRGB = true) const;

	// -----------------------------------------------------------------
	// Interpolation
	// -----------------------------------------------------------------

	/// @brief Linear interpolation between two colors.
	static constexpr FLinearColor Lerp(const FLinearColor& A, const FLinearColor& B, float Alpha)
	{
		return FLinearColor(
			A.R + (B.R - A.R) * Alpha,
			A.G + (B.G - A.G) * Alpha,
			A.B + (B.B - A.B) * Alpha,
			A.A + (B.A - A.A) * Alpha);
	}

	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	static const FLinearColor White;
	static const FLinearColor Black;
	static const FLinearColor Red;
	static const FLinearColor Green;
	static const FLinearColor Blue;
	static const FLinearColor Yellow;
	static const FLinearColor Transparent;
};

} // namespace Enigma
