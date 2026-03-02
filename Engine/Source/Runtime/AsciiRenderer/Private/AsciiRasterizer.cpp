// Copyright EnigmaEngine. All Rights Reserved.

/// @file AsciiRasterizer.cpp
/// @brief FRasterizer implementation - draw list pipeline with blend support.

#include "AsciiRasterizer.h"
#include "AsciiBackBuffer.h"
#include "RenderCore/AsciiSprite.h"
#include "SceneView/SceneView.h"

#include <algorithm>
#include <cmath>

namespace Enigma
{

// ---------------------------------------------------------------
// Public API
// ---------------------------------------------------------------
void FRasterizer::Clear()
{
	m_drawList.clear();
}

void FRasterizer::AddCommand(FDrawCommand cmd)
{
	m_drawList.push_back(std::move(cmd));
}

void FRasterizer::Rasterize(const FSceneView& view, FAsciiBackBuffer& buffer)
{
	if (m_drawList.empty())
	{
		return;
	}

	transformToScreen(view);
	frustumCull(buffer.GetWidth(), buffer.GetHeight());
	depthSort();
	rasterizeToBuffer(buffer);
}

// ---------------------------------------------------------------
// Pipeline stage 1: transform world -> screen coordinates
// ---------------------------------------------------------------
void FRasterizer::transformToScreen(const FSceneView& view)
{
	// Camera translation = 2D scroll offset for ASCII renderer.
	const auto& translation = view.ViewTransform.GetTranslation();
	int32_t camX = static_cast<int32_t>(std::floor(translation.X));
	int32_t camY = static_cast<int32_t>(std::floor(translation.Y));

	for (auto& cmd : m_drawList)
	{
		cmd.WorldX -= camX;
		cmd.WorldY -= camY;
	}
}

// ---------------------------------------------------------------
// Pipeline stage 2: frustum cull (discard entirely off-screen)
// ---------------------------------------------------------------
void FRasterizer::frustumCull(int32_t viewW, int32_t viewH)
{
	auto it = std::remove_if(m_drawList.begin(), m_drawList.end(),
		[viewW, viewH](const FDrawCommand& cmd)
		{
			// Compute bounding box in screen space.
			int32_t left = cmd.WorldX;
			int32_t top  = cmd.WorldY;
			int32_t w = 1, h = 1;

			switch (cmd.Type)
			{
			case EDrawCommandType::Cell:
				w = 1; h = 1;
				break;
			case EDrawCommandType::Sprite:
				if (cmd.Sprite)
				{
					w = cmd.Sprite->Width;
					h = cmd.Sprite->Height;
				}
				break;
			case EDrawCommandType::Text:
				w = static_cast<int32_t>(cmd.Text.size());
				h = 1;
				break;
			case EDrawCommandType::FillRect:
			case EDrawCommandType::DrawBox:
				w = cmd.Width;
				h = cmd.Height;
				break;
			}

			int32_t right  = left + w;
			int32_t bottom = top + h;

			// Entirely outside viewport?
			return (right <= 0 || left >= viewW || bottom <= 0 || top >= viewH);
		});

	m_drawList.erase(it, m_drawList.end());
}

// ---------------------------------------------------------------
// Pipeline stage 3: stable depth sort (ascending ZOrder = back-to-front)
// ---------------------------------------------------------------
void FRasterizer::depthSort()
{
	std::stable_sort(m_drawList.begin(), m_drawList.end(),
		[](const FDrawCommand& a, const FDrawCommand& b)
		{
			return a.ZOrder < b.ZOrder;
		});
}

// ---------------------------------------------------------------
// Pipeline stage 4: rasterize sorted commands into back buffer
// ---------------------------------------------------------------
void FRasterizer::rasterizeToBuffer(FAsciiBackBuffer& buffer)
{
	for (const auto& cmd : m_drawList)
	{
		switch (cmd.Type)
		{
		case EDrawCommandType::Cell:     rasterizeCell(cmd, buffer);     break;
		case EDrawCommandType::Sprite:   rasterizeSprite(cmd, buffer);   break;
		case EDrawCommandType::Text:     rasterizeText(cmd, buffer);     break;
		case EDrawCommandType::FillRect: rasterizeFillRect(cmd, buffer); break;
		case EDrawCommandType::DrawBox:  rasterizeDrawBox(cmd, buffer);  break;
		}
	}
}

// ---------------------------------------------------------------
// Per-command rasterization
// ---------------------------------------------------------------
void FRasterizer::rasterizeCell(const FDrawCommand& cmd, FAsciiBackBuffer& buffer)
{
	writeBlendedCell(cmd.WorldX, cmd.WorldY, cmd.Cell, cmd.BlendState, buffer);
}

void FRasterizer::rasterizeSprite(const FDrawCommand& cmd, FAsciiBackBuffer& buffer)
{
	if (!cmd.Sprite) return;

	const auto& sprite = *cmd.Sprite;
	for (int32_t sy = 0; sy < sprite.Height; ++sy)
	{
		for (int32_t sx = 0; sx < sprite.Width; ++sx)
		{
			const auto& cell = sprite.Cells[
				static_cast<size_t>(sy) * static_cast<size_t>(sprite.Width)
				+ static_cast<size_t>(sx)];
			writeBlendedCell(cmd.WorldX + sx, cmd.WorldY + sy,
			                 cell, cmd.BlendState, buffer);
		}
	}
}

void FRasterizer::rasterizeText(const FDrawCommand& cmd, FAsciiBackBuffer& buffer)
{
	int32_t x = cmd.WorldX;
	for (size_t i = 0; i < cmd.Text.size(); ++i)
	{
		FAsciiCell cell{cmd.Text[i], cmd.Fg, cmd.Bg};
		writeBlendedCell(x + static_cast<int32_t>(i), cmd.WorldY,
		                 cell, cmd.BlendState, buffer);
	}
}

void FRasterizer::rasterizeFillRect(const FDrawCommand& cmd, FAsciiBackBuffer& buffer)
{
	for (int32_t ry = 0; ry < cmd.Height; ++ry)
	{
		for (int32_t rx = 0; rx < cmd.Width; ++rx)
		{
			writeBlendedCell(cmd.WorldX + rx, cmd.WorldY + ry,
			                 cmd.Cell, cmd.BlendState, buffer);
		}
	}
}

void FRasterizer::rasterizeDrawBox(const FDrawCommand& cmd, FAsciiBackBuffer& buffer)
{
	if (cmd.Width <= 0 || cmd.Height <= 0) return;

	int32_t x0 = cmd.WorldX;
	int32_t y0 = cmd.WorldY;
	int32_t x1 = x0 + cmd.Width - 1;
	int32_t y1 = y0 + cmd.Height - 1;

	auto makeCell = [&](char ch) -> FAsciiCell
	{
		return FAsciiCell{ch, cmd.Fg, cmd.Bg};
	};

	// Corners
	writeBlendedCell(x0, y0, makeCell('+'), cmd.BlendState, buffer);
	if (cmd.Width > 1)
		writeBlendedCell(x1, y0, makeCell('+'), cmd.BlendState, buffer);
	if (cmd.Height > 1)
		writeBlendedCell(x0, y1, makeCell('+'), cmd.BlendState, buffer);
	if (cmd.Width > 1 && cmd.Height > 1)
		writeBlendedCell(x1, y1, makeCell('+'), cmd.BlendState, buffer);

	// Top and bottom edges
	for (int32_t x = x0 + 1; x < x1; ++x)
	{
		writeBlendedCell(x, y0, makeCell('-'), cmd.BlendState, buffer);
		if (cmd.Height > 1)
			writeBlendedCell(x, y1, makeCell('-'), cmd.BlendState, buffer);
	}

	// Left and right edges
	for (int32_t y = y0 + 1; y < y1; ++y)
	{
		writeBlendedCell(x0, y, makeCell('|'), cmd.BlendState, buffer);
		if (cmd.Width > 1)
			writeBlendedCell(x1, y, makeCell('|'), cmd.BlendState, buffer);
	}
}

// ---------------------------------------------------------------
// Blend helpers
// ---------------------------------------------------------------
void FRasterizer::writeBlendedCell(
	int32_t x, int32_t y, FAsciiCell srcCell,
	const FAsciiBlendState& state, FAsciiBackBuffer& buffer) const
{
	// Transparency: skip entirely if source cell is transparent.
	if (srcCell.IsTransparent())
	{
		return;
	}

	FAsciiCell dstCell = buffer.ReadCell(x, y);
	FAsciiCell result  = blendCell(srcCell, dstCell, state);
	buffer.WriteCell(x, y, result);
}

FAsciiCell FRasterizer::blendCell(
	FAsciiCell src, FAsciiCell dst, const FAsciiBlendState& state)
{
	FAsciiCell result = dst; // start from destination

	if (state.bWriteCharacter)
	{
		result.Character = blendChar(src.Character, dst.Character,
		                             state.CharacterOp, state.CustomCharacterFn);
	}
	if (state.bWriteForeground)
	{
		result.Foreground = blendColor(src.Foreground, dst.Foreground,
		                               state.ForegroundOp, state.CustomForegroundFn);
	}
	if (state.bWriteBackground)
	{
		result.Background = blendColor(src.Background, dst.Background,
		                               state.BackgroundOp, state.CustomBackgroundFn);
	}

	return result;
}

FColor FRasterizer::blendColor(
	FColor src, FColor dst, EBlendOp op,
	const std::function<FColor(FColor, FColor)>& customFn)
{
	switch (op)
	{
	case EBlendOp::Replace:
		return src;

	case EBlendOp::Add:
	{
		auto clamp8 = [](int v) -> uint8_t
		{
			return static_cast<uint8_t>(v > 255 ? 255 : (v < 0 ? 0 : v));
		};
		return FColor{
			clamp8(static_cast<int>(src.R) + static_cast<int>(dst.R)),
			clamp8(static_cast<int>(src.G) + static_cast<int>(dst.G)),
			clamp8(static_cast<int>(src.B) + static_cast<int>(dst.B)),
			src.A  // preserve source alpha
		};
	}

	case EBlendOp::Multiply:
	{
		return FColor{
			static_cast<uint8_t>((static_cast<int>(src.R) * static_cast<int>(dst.R)) / 255),
			static_cast<uint8_t>((static_cast<int>(src.G) * static_cast<int>(dst.G)) / 255),
			static_cast<uint8_t>((static_cast<int>(src.B) * static_cast<int>(dst.B)) / 255),
			src.A  // preserve source alpha
		};
	}

	case EBlendOp::Custom:
		if (customFn)
		{
			return customFn(src, dst);
		}
		return src; // fallback to Replace if no function
	}

	return src;
}

char FRasterizer::blendChar(
	char src, char dst, ECharBlendOp op,
	const std::function<char(char, char)>& customFn)
{
	switch (op)
	{
	case ECharBlendOp::Replace:
		return src;

	case ECharBlendOp::KeepNonSpace:
		return (dst != ' ') ? dst : src;

	case ECharBlendOp::Custom:
		if (customFn)
		{
			return customFn(src, dst);
		}
		return src; // fallback to Replace if no function
	}

	return src;
}

} // namespace Enigma
