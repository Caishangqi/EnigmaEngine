// Copyright EnigmaEngine. All Rights Reserved.

/// @file Color.cpp
/// @brief Implementation of FColor non-constexpr functions.

#include "Math/Color.h"
#include "Math/LinearColor.h"

#include <cstdio>

namespace Enigma
{

// -----------------------------------------------------------------
// Constants
// -----------------------------------------------------------------

const FColor FColor::White(255, 255, 255, 255);
const FColor FColor::Black(0, 0, 0, 255);
const FColor FColor::Red(255, 0, 0, 255);
const FColor FColor::Green(0, 255, 0, 255);
const FColor FColor::Blue(0, 0, 255, 255);
const FColor FColor::Yellow(255, 255, 0, 255);
const FColor FColor::Transparent(0, 0, 0, 0);

// -----------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------

FColor::FColor(const FLinearColor& LinearColor)
{
	// Delegate to FLinearColor::ToFColor for the actual conversion.
	*this = LinearColor.ToFColor(true);
}

// -----------------------------------------------------------------
// Conversion
// -----------------------------------------------------------------

FLinearColor FColor::ToLinearColor() const
{
	return FLinearColor(*this);
}

std::string FColor::ToHex() const
{
	char Buffer[10]; // "#RRGGBBAA" + null
	std::snprintf(Buffer, sizeof(Buffer), "#%02X%02X%02X%02X", R, G, B, A);
	return std::string(Buffer);
}

} // namespace Enigma
