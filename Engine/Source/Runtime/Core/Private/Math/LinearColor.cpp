// Copyright EnigmaEngine. All Rights Reserved.

/// @file LinearColor.cpp
/// @brief Implementation of FLinearColor non-constexpr functions.

#include "Math/LinearColor.h"
#include "Math/Color.h"

namespace
{

/// @brief Convert a single sRGB channel [0,255] to linear [0,1].
/// Standard sRGB transfer function with linear segment below 0.04045.
float SRGBToLinear(float S)
{
	if (S <= 0.04045f)
	{
		return S / 12.92f;
	}
	return Enigma::FMath::Pow((S + 0.055f) / 1.055f, 2.4f);
}

/// @brief Convert a single linear [0,1] channel to sRGB [0,1].
/// Inverse of the standard sRGB transfer function.
float LinearToSRGB(float L)
{
	if (L <= 0.0031308f)
	{
		return L * 12.92f;
	}
	return 1.055f * Enigma::FMath::Pow(L, 1.0f / 2.4f) - 0.055f;
}

} // anonymous namespace

namespace Enigma
{

// -----------------------------------------------------------------
// Constants
// -----------------------------------------------------------------

const FLinearColor FLinearColor::White(1.0f, 1.0f, 1.0f, 1.0f);
const FLinearColor FLinearColor::Black(0.0f, 0.0f, 0.0f, 1.0f);
const FLinearColor FLinearColor::Red(1.0f, 0.0f, 0.0f, 1.0f);
const FLinearColor FLinearColor::Green(0.0f, 1.0f, 0.0f, 1.0f);
const FLinearColor FLinearColor::Blue(0.0f, 0.0f, 1.0f, 1.0f);
const FLinearColor FLinearColor::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
const FLinearColor FLinearColor::Transparent(0.0f, 0.0f, 0.0f, 0.0f);

// -----------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------

FLinearColor::FLinearColor(const FColor& Color)
{
	// Convert 8-bit sRGB channels to linear float.
	const float InvByte = 1.0f / 255.0f;
	R = SRGBToLinear(static_cast<float>(Color.R) * InvByte);
	G = SRGBToLinear(static_cast<float>(Color.G) * InvByte);
	B = SRGBToLinear(static_cast<float>(Color.B) * InvByte);
	A = static_cast<float>(Color.A) * InvByte; // Alpha is linear.
}

// -----------------------------------------------------------------
// Conversion
// -----------------------------------------------------------------

FColor FLinearColor::ToFColor(bool bSRGB) const
{
	// Clamp to [0,1] then convert.
	const float CR = FMath::Clamp(R, 0.0f, 1.0f);
	const float CG = FMath::Clamp(G, 0.0f, 1.0f);
	const float CB = FMath::Clamp(B, 0.0f, 1.0f);
	const float CA = FMath::Clamp(A, 0.0f, 1.0f);

	if (bSRGB)
	{
		return FColor(
			static_cast<uint8_t>(LinearToSRGB(CR) * 255.0f + 0.5f),
			static_cast<uint8_t>(LinearToSRGB(CG) * 255.0f + 0.5f),
			static_cast<uint8_t>(LinearToSRGB(CB) * 255.0f + 0.5f),
			static_cast<uint8_t>(CA * 255.0f + 0.5f));
	}

	return FColor(
		static_cast<uint8_t>(CR * 255.0f + 0.5f),
		static_cast<uint8_t>(CG * 255.0f + 0.5f),
		static_cast<uint8_t>(CB * 255.0f + 0.5f),
		static_cast<uint8_t>(CA * 255.0f + 0.5f));
}

} // namespace Enigma
