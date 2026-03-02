// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiCell.h
/// @brief Single ASCII cell with character and foreground/background colors.

#include "RenderCoreAPI.generated.h"
#include "Math/Color.h"

namespace Enigma
{

/// A single cell in the ASCII rendering buffer.
/// Contains a character and foreground/background colors.
struct RENDERCORE_API FAsciiCell
{
	char   Character  = ' ';
	FColor Foreground = FColor::White;
	FColor Background = FColor::Black;

	constexpr FAsciiCell() = default;

	constexpr FAsciiCell(char ch, FColor fg, FColor bg)
		: Character(ch), Foreground(fg), Background(bg)
	{
	}

	/// Transparent cell marker: Character == '\0'.
	/// Transparent cells are skipped during blending.
	constexpr bool IsTransparent() const { return Character == '\0'; }

	constexpr bool operator==(const FAsciiCell& other) const
	{
		return Character == other.Character
			&& Foreground == other.Foreground
			&& Background == other.Background;
	}

	constexpr bool operator!=(const FAsciiCell& other) const
	{
		return !(*this == other);
	}
};

} // namespace Enigma
