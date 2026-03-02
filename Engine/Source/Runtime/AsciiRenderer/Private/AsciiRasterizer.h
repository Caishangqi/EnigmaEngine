// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file AsciiRasterizer.h
/// @brief FDrawCommand + FRasterizer - draw list accumulation and rasterization pipeline.

#include "RenderCore/AsciiBlendState.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Enigma
{

// Forward declarations
struct FAsciiSprite;
struct FSceneView;
class FAsciiBackBuffer;

/// Draw command type tag.
enum class EDrawCommandType : uint8_t
{
	Cell,
	Sprite,
	Text,
	FillRect,
	DrawBox
};

/// A single deferred draw command with captured blend state.
/// Accumulated by FAsciiRendererModule::Draw*(), processed by FRasterizer::Rasterize().
struct FDrawCommand
{
	EDrawCommandType Type = EDrawCommandType::Cell;
	int32_t WorldX = 0;
	int32_t WorldY = 0;
	int32_t ZOrder = 0;
	FAsciiBlendState BlendState;    ///< Captured at submission time (REQ-17)

	// --- Per-type data ---
	FAsciiCell          Cell;                   ///< Cell, FillRect
	const FAsciiSprite* Sprite = nullptr;       ///< Sprite (non-owning)
	std::string         Text;                   ///< Text (owning copy)
	FColor              Fg = FColor::White;     ///< Text, DrawBox
	FColor              Bg = FColor::Black;     ///< Text, DrawBox
	int32_t             Width  = 0;             ///< FillRect, DrawBox
	int32_t             Height = 0;             ///< FillRect, DrawBox
};

/// Rasterization pipeline: accumulate draw commands, then rasterize into back buffer.
/// Pipeline: transformToScreen -> frustumCull -> depthSort (stable) -> rasterizeToBuffer.
class FRasterizer
{
public:
	/// Clear the draw list for a new frame.
	void Clear();

	/// Append a draw command. O(1) amortized.
	void AddCommand(FDrawCommand cmd);

	/// Execute the full rasterization pipeline.
	void Rasterize(const FSceneView& view, FAsciiBackBuffer& buffer);

private:
	std::vector<FDrawCommand> m_drawList;

	// --- Pipeline stages ---
	void transformToScreen(const FSceneView& view);
	void frustumCull(int32_t viewW, int32_t viewH);
	void depthSort();
	void rasterizeToBuffer(FAsciiBackBuffer& buffer);

	// --- Per-command rasterization ---
	void rasterizeCell(const FDrawCommand& cmd, FAsciiBackBuffer& buffer);
	void rasterizeSprite(const FDrawCommand& cmd, FAsciiBackBuffer& buffer);
	void rasterizeText(const FDrawCommand& cmd, FAsciiBackBuffer& buffer);
	void rasterizeFillRect(const FDrawCommand& cmd, FAsciiBackBuffer& buffer);
	void rasterizeDrawBox(const FDrawCommand& cmd, FAsciiBackBuffer& buffer);

	// --- Blend helpers ---
	void writeBlendedCell(int32_t x, int32_t y, FAsciiCell srcCell,
	                      const FAsciiBlendState& state,
	                      FAsciiBackBuffer& buffer) const;

	static FAsciiCell blendCell(FAsciiCell src, FAsciiCell dst,
	                            const FAsciiBlendState& state);
	static FColor blendColor(FColor src, FColor dst, EBlendOp op,
	                         const std::function<FColor(FColor, FColor)>& customFn);
	static char blendChar(char src, char dst, ECharBlendOp op,
	                      const std::function<char(char, char)>& customFn);
};

} // namespace Enigma
