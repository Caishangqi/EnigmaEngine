// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiBackBuffer.cpp
/// @brief FAsciiBackBuffer implementation.

#include "AsciiBackBuffer.h"

#include <algorithm>

namespace Enigma
{

void FAsciiBackBuffer::Allocate(int32_t width, int32_t height)
{
	m_width  = (width > 0)  ? width  : 0;
	m_height = (height > 0) ? height : 0;

	const auto totalCells = static_cast<size_t>(m_width) * static_cast<size_t>(m_height);
	m_cells.assign(totalCells, FAsciiCell{});
}

void FAsciiBackBuffer::Clear(FAsciiCell clearCell)
{
	std::fill(m_cells.begin(), m_cells.end(), clearCell);
}

void FAsciiBackBuffer::WriteCell(int32_t x, int32_t y, FAsciiCell cell)
{
	if (x < 0 || x >= m_width || y < 0 || y >= m_height)
	{
		return; // silently ignore out-of-bounds
	}
	m_cells[static_cast<size_t>(y) * static_cast<size_t>(m_width) + static_cast<size_t>(x)] = cell;
}

FAsciiCell FAsciiBackBuffer::ReadCell(int32_t x, int32_t y) const
{
	if (x < 0 || x >= m_width || y < 0 || y >= m_height)
	{
		return FAsciiCell{}; // default cell for out-of-bounds
	}
	return m_cells[static_cast<size_t>(y) * static_cast<size_t>(m_width) + static_cast<size_t>(x)];
}

} // namespace Enigma
