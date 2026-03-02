// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file ClassicConsoleBackend.h
/// @brief FClassicConsoleBackend - 16-color WriteConsoleOutput backend.

#include "AsciiRenderer/AsciiRenderBackend.h"

#include <vector>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace Enigma
{

/// Classic console rendering backend using WriteConsoleOutput.
/// Converts FAsciiCell[] to CHAR_INFO[] with RGB-to-16-color Euclidean distance mapping.
/// Supports custom palette via GetConsoleScreenBufferInfoEx.
class FClassicConsoleBackend : public IAsciiRenderBackend
{
public:
	bool Initialize(void* consoleOutputHandle) override;
	void Present(const FAsciiCell* backBuffer, int32_t width, int32_t height) override;
	std::string_view GetName() const override { return "Classic"; }
	int32_t GetMaxColorCount() const override { return 16; }
	void Shutdown() override;

private:
#ifdef _WIN32
	/// Map an FColor to the nearest 16-color console attribute index.
	WORD MapColorToIndex(FColor color) const;

	/// Load the current console color palette.
	void LoadPalette();

	HANDLE              m_consoleHandle = INVALID_HANDLE_VALUE;
	COLORREF            m_palette[16]   = {};
	std::vector<CHAR_INFO> m_charInfoBuffer;
#endif
};

} // namespace Enigma
