// Copyright EnigmaEngine. All Rights Reserved.

/// @file ClassicConsoleBackend.cpp
/// @brief FClassicConsoleBackend implementation - 16-color WriteConsoleOutput.

#include "ClassicConsoleBackend.h"

#include <cstdio>

namespace Enigma
{

#ifdef _WIN32

// Default Windows console 16-color palette (COLORREF = 0x00BBGGRR).
static constexpr COLORREF s_defaultPalette[16] =
{
	0x00000000, // 0  Black
	0x00800000, // 1  Dark Blue
	0x00008000, // 2  Dark Green
	0x00808000, // 3  Dark Cyan
	0x00000080, // 4  Dark Red
	0x00800080, // 5  Dark Magenta
	0x00008080, // 6  Dark Yellow
	0x00C0C0C0, // 7  Gray
	0x00808080, // 8  Dark Gray
	0x00FF0000, // 9  Blue
	0x0000FF00, // 10 Green
	0x00FFFF00, // 11 Cyan
	0x000000FF, // 12 Red
	0x00FF00FF, // 13 Magenta
	0x0000FFFF, // 14 Yellow
	0x00FFFFFF  // 15 White
};

bool FClassicConsoleBackend::Initialize(void* consoleOutputHandle)
{
	if (!consoleOutputHandle)
	{
		return false;
	}

	m_consoleHandle = static_cast<HANDLE>(consoleOutputHandle);
	LoadPalette();
	return true;
}

void FClassicConsoleBackend::LoadPalette()
{
	// Try to read the actual console palette.
	CONSOLE_SCREEN_BUFFER_INFOEX csbiex = {};
	csbiex.cbSize = sizeof(csbiex);

	if (::GetConsoleScreenBufferInfoEx(m_consoleHandle, &csbiex))
	{
		for (int i = 0; i < 16; ++i)
		{
			m_palette[i] = csbiex.ColorTable[i];
		}
	}
	else
	{
		// Fallback to default palette.
		for (int i = 0; i < 16; ++i)
		{
			m_palette[i] = s_defaultPalette[i];
		}
	}
}

WORD FClassicConsoleBackend::MapColorToIndex(FColor color) const
{
	int bestIndex = 0;
	int bestDist  = INT_MAX;

	for (int i = 0; i < 16; ++i)
	{
		// COLORREF is 0x00BBGGRR
		int pr = static_cast<int>(m_palette[i] & 0xFF);
		int pg = static_cast<int>((m_palette[i] >> 8) & 0xFF);
		int pb = static_cast<int>((m_palette[i] >> 16) & 0xFF);

		int dr = static_cast<int>(color.R) - pr;
		int dg = static_cast<int>(color.G) - pg;
		int db = static_cast<int>(color.B) - pb;

		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist)
		{
			bestDist  = dist;
			bestIndex = i;
		}
	}

	return static_cast<WORD>(bestIndex);
}

void FClassicConsoleBackend::Present(
	const FAsciiCell* backBuffer, int32_t width, int32_t height)
{
	if (!backBuffer || width <= 0 || height <= 0)
	{
		return;
	}

	const auto totalCells = static_cast<size_t>(width) * static_cast<size_t>(height);

	// Resize CHAR_INFO buffer if needed.
	if (m_charInfoBuffer.size() != totalCells)
	{
		m_charInfoBuffer.resize(totalCells);
	}

	// Convert FAsciiCell[] to CHAR_INFO[].
	for (size_t i = 0; i < totalCells; ++i)
	{
		const auto& cell = backBuffer[i];
		auto& ci = m_charInfoBuffer[i];

		ci.Char.AsciiChar = cell.Character;
		WORD fgAttr = MapColorToIndex(cell.Foreground);
		WORD bgAttr = MapColorToIndex(cell.Background);
		ci.Attributes = fgAttr | (bgAttr << 4);
	}

	// Single WriteConsoleOutput call (atomic swap).
	COORD bufferSize = { static_cast<SHORT>(width), static_cast<SHORT>(height) };
	COORD bufferCoord = { 0, 0 };
	SMALL_RECT writeRegion = {
		0, 0,
		static_cast<SHORT>(width - 1),
		static_cast<SHORT>(height - 1)
	};

	::WriteConsoleOutputA(
		m_consoleHandle,
		m_charInfoBuffer.data(),
		bufferSize,
		bufferCoord,
		&writeRegion);
}

void FClassicConsoleBackend::Shutdown()
{
	m_charInfoBuffer.clear();
	m_charInfoBuffer.shrink_to_fit();
	m_consoleHandle = INVALID_HANDLE_VALUE;
}

#else

// Stub implementations for non-Windows platforms.
bool FClassicConsoleBackend::Initialize(void*) { return false; }
void FClassicConsoleBackend::Present(const FAsciiCell*, int32_t, int32_t) {}
void FClassicConsoleBackend::Shutdown() {}

#endif // _WIN32

} // namespace Enigma
