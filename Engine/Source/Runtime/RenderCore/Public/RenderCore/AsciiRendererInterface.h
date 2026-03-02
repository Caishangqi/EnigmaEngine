// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiRendererInterface.h
/// @brief ASCII renderer sub-interface - draw commands + blend state.
/// Game code accesses this via FModuleManager::Get().GetModuleChecked<IAsciiRendererModule>("Renderer").

#include "RenderCoreAPI.generated.h"
#include "RenderCore/RendererInterface.h"
#include "RenderCore/AsciiBlendState.h"

#include <cstdint>

namespace Enigma
{

// Forward declarations
struct FColor;
struct FAsciiCell;
struct FAsciiSprite;

/// ASCII renderer sub-interface - draw commands + blend state.
/// A future DX12 renderer defines its own sub-interface (e.g., IDX12RendererModule).
class IAsciiRendererModule : public IRendererModule
{
public:
	// --- Draw commands (world-space, deferred to EndFrame) ---

	/// Draw a single cell at world position with given Z-order.
	virtual void DrawCell(int32_t worldX, int32_t worldY, int32_t zOrder,
	                      FAsciiCell cell) = 0;

	/// Draw a sprite at world position with given Z-order.
	virtual void DrawSprite(int32_t worldX, int32_t worldY, int32_t zOrder,
	                        const FAsciiSprite& sprite) = 0;

	/// Draw text at world position with given Z-order and colors.
	virtual void DrawText(int32_t worldX, int32_t worldY, int32_t zOrder,
	                      const char* text, FColor fg, FColor bg) = 0;

	/// Fill a rectangle with a cell at world position with given Z-order.
	virtual void FillRect(int32_t worldX, int32_t worldY, int32_t width,
	                      int32_t height, int32_t zOrder, FAsciiCell cell) = 0;

	/// Draw a box border at world position with given Z-order and colors.
	virtual void DrawBox(int32_t worldX, int32_t worldY, int32_t width,
	                     int32_t height, int32_t zOrder, FColor fg, FColor bg) = 0;

	// --- Blend state (REQ-17) ---

	/// Set the blend state for subsequent draw commands until next change or BeginFrame.
	virtual void SetBlendState(const FAsciiBlendState& state) = 0;
};

} // namespace Enigma
