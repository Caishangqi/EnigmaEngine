// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiSprite.h
/// @brief 2D grid of FAsciiCell for sprite rendering.

#include "RenderCoreAPI.generated.h"
#include "RenderCore/AsciiCell.h"

#include <cstdint>
#include <vector>

namespace Enigma
{

/// A 2D grid of ASCII cells, stored in row-major order (Width x Height).
/// Used for multi-cell sprite rendering via IAsciiRendererModule::DrawSprite().
struct RENDERCORE_API FAsciiSprite
{
	std::vector<FAsciiCell> Cells;  ///< Row-major grid (Width x Height)
	int32_t Width  = 0;
	int32_t Height = 0;

	FAsciiSprite() = default;

	/// Construct a sprite with the given dimensions.
	/// All cells are default-initialized (space, White fg, Black bg).
	FAsciiSprite(int32_t w, int32_t h);

	/// Access a cell at (x, y). Asserts on out-of-bounds.
	FAsciiCell& At(int32_t x, int32_t y);

	/// Access a cell at (x, y) (const). Asserts on out-of-bounds.
	const FAsciiCell& At(int32_t x, int32_t y) const;
};

} // namespace Enigma
