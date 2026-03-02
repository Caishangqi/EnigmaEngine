// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiBackBuffer.h
/// @brief Off-screen character cell buffer for the ASCII renderer.

#include "RenderCore/AsciiCell.h"

#include <cstdint>
#include <vector>

namespace Enigma
{

/// Manages the off-screen character cell buffer.
/// Provides allocate, clear, read/write, and dimension queries.
/// Used internally by FAsciiRendererModule; not exported.
class FAsciiBackBuffer
{
public:
	/// Allocate (or reallocate) the buffer to the given dimensions.
	/// Clears all cells to the default FAsciiCell.
	void Allocate(int32_t width, int32_t height);

	/// Clear all cells to the given value.
	void Clear(FAsciiCell clearCell = FAsciiCell{' ', FColor::White, FColor::Black});

	/// Write a cell at (x, y). Out-of-bounds writes are silently ignored.
	void WriteCell(int32_t x, int32_t y, FAsciiCell cell);

	/// Read a cell at (x, y). Out-of-bounds reads return default FAsciiCell.
	FAsciiCell ReadCell(int32_t x, int32_t y) const;

	/// Buffer width in cells.
	int32_t GetWidth() const { return m_width; }

	/// Buffer height in cells.
	int32_t GetHeight() const { return m_height; }

	/// Raw pointer to cell data (row-major) for backend Present().
	const FAsciiCell* GetData() const { return m_cells.data(); }

private:
	std::vector<FAsciiCell> m_cells;
	int32_t m_width  = 0;
	int32_t m_height = 0;
};

} // namespace Enigma
