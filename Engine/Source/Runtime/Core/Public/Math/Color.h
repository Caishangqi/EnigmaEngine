// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file Color.h
/// @brief 8-bit sRGB color type (R, G, B, A).

#include "CoreAPI.generated.h"

#include <cstdint>
#include <string>

namespace Enigma
{

struct FLinearColor;

/// @brief 8-bit sRGB color.
///
/// Components are uint8_t in [0, 255]. Alpha defaults to 255 (opaque).
/// Use ToLinearColor() to convert to linear-space for rendering math.
struct CORE_API FColor
{
	/// Red channel.
	uint8_t R;

	/// Green channel.
	uint8_t G;

	/// Blue channel.
	uint8_t B;

	/// Alpha channel.
	uint8_t A;

	// -----------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------

	/// @brief Default constructor. Black opaque (0,0,0,255).
	constexpr FColor()
		: R(0), G(0), B(0), A(255)
	{
	}

	/// @brief Construct from explicit components (A defaults to 255).
	constexpr FColor(uint8_t InR, uint8_t InG, uint8_t InB, uint8_t InA = 255)
		: R(InR), G(InG), B(InB), A(InA)
	{
	}

	/// @brief Construct from a linear-space FLinearColor (linear to sRGB).
	explicit FColor(const FLinearColor& LinearColor);

	// -----------------------------------------------------------------
	// Comparison
	// -----------------------------------------------------------------

	/// @brief Exact equality.
	constexpr bool operator==(const FColor& Other) const
	{
		return R == Other.R && G == Other.G && B == Other.B && A == Other.A;
	}

	/// @brief Exact inequality.
	constexpr bool operator!=(const FColor& Other) const
	{
		return !(*this == Other);
	}

	// -----------------------------------------------------------------
	// Conversion
	// -----------------------------------------------------------------

	/// @brief Convert to linear-space FLinearColor (sRGB to linear).
	FLinearColor ToLinearColor() const;

	/// @brief Convert to hex string in "#RRGGBBAA" format.
	std::string ToHex() const;

	// -----------------------------------------------------------------
	// Constants
	// -----------------------------------------------------------------

	static const FColor White;
	static const FColor Black;
	static const FColor Red;
	static const FColor Green;
	static const FColor Blue;
	static const FColor Yellow;
	static const FColor Transparent;
};

} // namespace Enigma
