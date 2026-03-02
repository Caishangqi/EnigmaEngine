// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file VTConsoleBackend.h
/// @brief FVTConsoleBackend - 256-color ANSI escape sequence backend with dirty-rect.

#include "AsciiRenderer/AsciiRenderBackend.h"

#include <string>
#include <vector>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace Enigma
{

/// VT console rendering backend using ANSI escape sequences.
/// Supports 256-color via ESC[38;5;nm / ESC[48;5;nm.
/// Dirty-rect optimization: only emits changed cells.
/// Batches consecutive same-color cells for efficiency.
class FVTConsoleBackend : public IAsciiRenderBackend
{
public:
	bool Initialize(void* consoleOutputHandle) override;
	void Present(const FAsciiCell* backBuffer, int32_t width, int32_t height) override;
	std::string_view GetName() const override { return "VT"; }
	int32_t GetMaxColorCount() const override { return 256; }
	void Shutdown() override;

private:
	/// Map an FColor to the nearest 256-color index.
	/// Checks standard (0-15), color cube (16-231), grayscale (232-255).
	static int32_t MapColorTo256(FColor color);

	/// Append cursor-move escape sequence (1-based row/col).
	static void AppendCursorMove(std::string& out, int32_t row, int32_t col);

	/// Append SGR for fg + bg 256-color.
	static void AppendSGR(std::string& out, int32_t fgIndex, int32_t bgIndex);

#ifdef _WIN32
	HANDLE m_consoleHandle = INVALID_HANDLE_VALUE;
	DWORD  m_originalMode  = 0;
#endif

	std::vector<FAsciiCell> m_prevBuffer;
	int32_t m_prevWidth  = 0;
	int32_t m_prevHeight = 0;
	std::string m_outputBuffer;
};

} // namespace Enigma
