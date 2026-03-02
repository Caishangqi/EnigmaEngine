// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiRenderBackend.h
/// @brief IAsciiRenderBackend interface and EAsciiRenderBackendType enum.
/// Strategy pattern for console output backends (Classic / VT).

#include "AsciiRendererAPI.generated.h"
#include "RenderCore/AsciiCell.h"

#include <cstdint>
#include <string_view>

namespace Enigma
{

/// Backend selection for console rendering.
enum class EAsciiRenderBackendType : uint8_t
{
	Auto,       ///< Detect VT support, fallback to Classic.
	Classic,    ///< WriteConsoleOutput, 16-color palette.
	VT          ///< ANSI escape sequences, 256-color.
};

/// Pure virtual interface for console rendering backends.
/// Classic backend uses WriteConsoleOutput (16-color).
/// VT backend uses ANSI escape sequences (256-color).
class ASCIIRENDERER_API IAsciiRenderBackend
{
public:
	virtual ~IAsciiRenderBackend() = default;

	/// Initialize the backend with a platform console output handle.
	/// @param consoleOutputHandle Platform-specific handle (HANDLE on Win32).
	/// @return true if initialization succeeded.
	virtual bool Initialize(void* consoleOutputHandle) = 0;

	/// Present the back buffer contents to the console.
	/// @param backBuffer Pointer to row-major FAsciiCell array.
	/// @param width Buffer width in cells.
	/// @param height Buffer height in cells.
	virtual void Present(const FAsciiCell* backBuffer,
	                     int32_t width, int32_t height) = 0;

	/// Human-readable backend name (e.g. "Classic", "VT").
	virtual std::string_view GetName() const = 0;

	/// Maximum number of distinct colors this backend supports.
	virtual int32_t GetMaxColorCount() const = 0;

	/// Shutdown and release platform resources.
	virtual void Shutdown() = 0;
};

} // namespace Enigma
