// Copyright EnigmaEngine. All Rights Reserved.

/// @file VTConsoleBackend.cpp
/// @brief FVTConsoleBackend implementation - 256-color ANSI with dirty-rect.

#include "VTConsoleBackend.h"

#include <algorithm>
#include <charconv>
#include <climits>
#include <cstdio>

namespace Enigma
{

// Standard ANSI 16-color palette (xterm defaults).
static constexpr uint8_t s_standard16[16][3] =
{
	{  0,   0,   0}, // 0  Black
	{128,   0,   0}, // 1  Maroon
	{  0, 128,   0}, // 2  Green
	{128, 128,   0}, // 3  Olive
	{  0,   0, 128}, // 4  Navy
	{128,   0, 128}, // 5  Purple
	{  0, 128, 128}, // 6  Teal
	{192, 192, 192}, // 7  Silver
	{128, 128, 128}, // 8  Gray
	{255,   0,   0}, // 9  Red
	{  0, 255,   0}, // 10 Lime
	{255, 255,   0}, // 11 Yellow
	{  0,   0, 255}, // 12 Blue
	{255,   0, 255}, // 13 Fuchsia
	{  0, 255, 255}, // 14 Aqua
	{255, 255, 255}  // 15 White
};

// Color cube levels: 6 values for each channel (indices 16-231).
static constexpr uint8_t s_cubeLevels[6] = { 0, 95, 135, 175, 215, 255 };

/// Find the nearest index in s_cubeLevels for a channel value.
static int32_t NearestCubeIndex(uint8_t value)
{
	int32_t best = 0;
	int32_t bestDist = INT_MAX;
	for (int32_t i = 0; i < 6; ++i)
	{
		int32_t d = static_cast<int32_t>(value) - static_cast<int32_t>(s_cubeLevels[i]);
		int32_t dist = d * d;
		if (dist < bestDist)
		{
			bestDist = dist;
			best = i;
		}
	}
	return best;
}

// ---------------------------------------------------------------------------
// MapColorTo256
// ---------------------------------------------------------------------------
int32_t FVTConsoleBackend::MapColorTo256(FColor color)
{
	int bestIndex = 0;
	int bestDist  = INT_MAX;

	// 1) Check standard 16 colors (indices 0-15).
	for (int i = 0; i < 16; ++i)
	{
		int dr = static_cast<int>(color.R) - static_cast<int>(s_standard16[i][0]);
		int dg = static_cast<int>(color.G) - static_cast<int>(s_standard16[i][1]);
		int db = static_cast<int>(color.B) - static_cast<int>(s_standard16[i][2]);
		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist)
		{
			bestDist  = dist;
			bestIndex = i;
		}
	}

	// 2) Check color cube (indices 16-231): index = 16 + 36*r + 6*g + b.
	{
		int ri = NearestCubeIndex(color.R);
		int gi = NearestCubeIndex(color.G);
		int bi = NearestCubeIndex(color.B);

		int dr = static_cast<int>(color.R) - static_cast<int>(s_cubeLevels[ri]);
		int dg = static_cast<int>(color.G) - static_cast<int>(s_cubeLevels[gi]);
		int db = static_cast<int>(color.B) - static_cast<int>(s_cubeLevels[bi]);
		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist)
		{
			bestDist  = dist;
			bestIndex = 16 + 36 * ri + 6 * gi + bi;
		}
	}

	// 3) Check grayscale ramp (indices 232-255): 24 shades, 8 + i*10.
	{
		int gray = (static_cast<int>(color.R) + static_cast<int>(color.G)
		            + static_cast<int>(color.B)) / 3;
		// Clamp to grayscale range [8, 238].
		int gi = (gray - 8) / 10;
		gi = std::clamp(gi, 0, 23);

		int grayVal = 8 + gi * 10;
		int dr = static_cast<int>(color.R) - grayVal;
		int dg = static_cast<int>(color.G) - grayVal;
		int db = static_cast<int>(color.B) - grayVal;
		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist)
		{
			bestDist  = dist;
			bestIndex = 232 + gi;
		}
	}

	return bestIndex;
}

// ---------------------------------------------------------------------------
// Escape sequence helpers
// ---------------------------------------------------------------------------
void FVTConsoleBackend::AppendCursorMove(std::string& out, int32_t row, int32_t col)
{
	// ESC[{row};{col}H  (1-based)
	char buf[32];
	auto [p1, ec1] = std::to_chars(buf, buf + sizeof(buf), row);
	*p1++ = ';';
	auto [p2, ec2] = std::to_chars(p1, buf + sizeof(buf), col);

	out += "\x1b[";
	out.append(buf, static_cast<size_t>(p2 - buf));
	out += 'H';
}

void FVTConsoleBackend::AppendSGR(std::string& out, int32_t fgIndex, int32_t bgIndex)
{
	// ESC[38;5;{fg};48;5;{bg}m
	char buf[32];
	out += "\x1b[38;5;";
	auto [p1, ec1] = std::to_chars(buf, buf + sizeof(buf), fgIndex);
	out.append(buf, static_cast<size_t>(p1 - buf));
	out += ";48;5;";
	auto [p2, ec2] = std::to_chars(buf, buf + sizeof(buf), bgIndex);
	out.append(buf, static_cast<size_t>(p2 - buf));
	out += 'm';
}

// ---------------------------------------------------------------------------
// Initialize / Shutdown
// ---------------------------------------------------------------------------
#ifdef _WIN32

bool FVTConsoleBackend::Initialize(void* consoleOutputHandle)
{
	if (!consoleOutputHandle)
	{
		return false;
	}

	m_consoleHandle = static_cast<HANDLE>(consoleOutputHandle);

	// Save original mode and enable VT processing.
	if (!::GetConsoleMode(m_consoleHandle, &m_originalMode))
	{
		return false;
	}

	DWORD newMode = m_originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING
	                | DISABLE_NEWLINE_AUTO_RETURN;
	if (!::SetConsoleMode(m_consoleHandle, newMode))
	{
		return false;
	}

	// Hide cursor for cleaner rendering.
	CONSOLE_CURSOR_INFO cci = {};
	cci.dwSize   = 1;
	cci.bVisible = FALSE;
	::SetConsoleCursorInfo(m_consoleHandle, &cci);

	return true;
}

void FVTConsoleBackend::Shutdown()
{
	if (m_consoleHandle != INVALID_HANDLE_VALUE)
	{
		// Reset SGR and show cursor.
		DWORD written = 0;
		const char* reset = "\x1b[0m\x1b[?25h";
		::WriteConsoleA(m_consoleHandle, reset, static_cast<DWORD>(strlen(reset)),
		                &written, nullptr);

		// Restore original console mode.
		::SetConsoleMode(m_consoleHandle, m_originalMode);
		m_consoleHandle = INVALID_HANDLE_VALUE;
	}

	m_prevBuffer.clear();
	m_prevBuffer.shrink_to_fit();
	m_prevWidth  = 0;
	m_prevHeight = 0;
	m_outputBuffer.clear();
	m_outputBuffer.shrink_to_fit();
}

// ---------------------------------------------------------------------------
// Present - dirty-rect + same-color batching
// ---------------------------------------------------------------------------
void FVTConsoleBackend::Present(
	const FAsciiCell* backBuffer, int32_t width, int32_t height)
{
	if (!backBuffer || width <= 0 || height <= 0)
	{
		return;
	}

	const auto totalCells = static_cast<size_t>(width) * static_cast<size_t>(height);

	// If dimensions changed, invalidate previous buffer (full redraw).
	if (width != m_prevWidth || height != m_prevHeight)
	{
		m_prevBuffer.assign(totalCells, FAsciiCell{'\0', FColor::Black, FColor::Black});
		m_prevWidth  = width;
		m_prevHeight = height;
	}

	m_outputBuffer.clear();
	// Reserve a reasonable estimate to avoid frequent reallocations.
	m_outputBuffer.reserve(totalCells * 4);

	int32_t curFg = -1;
	int32_t curBg = -1;

	for (int32_t y = 0; y < height; ++y)
	{
		// Track whether we need a cursor move for this row segment.
		bool needCursorMove = true;

		for (int32_t x = 0; x < width; ++x)
		{
			size_t idx = static_cast<size_t>(y) * static_cast<size_t>(width)
			             + static_cast<size_t>(x);
			const auto& cell = backBuffer[idx];
			const auto& prev = m_prevBuffer[idx];

			// Dirty-rect: skip unchanged cells.
			if (cell == prev)
			{
				needCursorMove = true;
				continue;
			}

			// Emit cursor move if needed.
			if (needCursorMove)
			{
				AppendCursorMove(m_outputBuffer, y + 1, x + 1);
				needCursorMove = false;
				// Force SGR re-emit after cursor move.
				curFg = -1;
				curBg = -1;
			}

			// Map colors.
			int32_t fgIdx = MapColorTo256(cell.Foreground);
			int32_t bgIdx = MapColorTo256(cell.Background);

			// Emit SGR only if colors changed (batching).
			if (fgIdx != curFg || bgIdx != curBg)
			{
				AppendSGR(m_outputBuffer, fgIdx, bgIdx);
				curFg = fgIdx;
				curBg = bgIdx;
			}

			// Emit character.
			m_outputBuffer += cell.Character;
		}
	}

	// Flush output in a single WriteConsole call.
	if (!m_outputBuffer.empty())
	{
		DWORD written = 0;
		::WriteConsoleA(m_consoleHandle, m_outputBuffer.data(),
		                static_cast<DWORD>(m_outputBuffer.size()),
		                &written, nullptr);
	}

	// Update previous buffer.
	std::copy(backBuffer, backBuffer + totalCells, m_prevBuffer.begin());
}

#else

// Stub implementations for non-Windows platforms.
bool FVTConsoleBackend::Initialize(void*) { return false; }
void FVTConsoleBackend::Present(const FAsciiCell*, int32_t, int32_t) {}
void FVTConsoleBackend::Shutdown() {}

#endif // _WIN32

} // namespace Enigma
