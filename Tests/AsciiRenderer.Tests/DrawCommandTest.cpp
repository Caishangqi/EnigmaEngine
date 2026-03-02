// Copyright EnigmaEngine. All Rights Reserved.

/// @file DrawCommandTest.cpp
/// @brief Unit tests for draw command rasterization: Cell, Text, Sprite,
///        FillRect, DrawBox.

#include <gtest/gtest.h>

#include "AsciiRasterizer.h"
#include "AsciiBackBuffer.h"
#include "RenderCore/AsciiCell.h"
#include "RenderCore/AsciiSprite.h"
#include "RenderCore/AsciiBlendState.h"
#include "SceneView/SceneView.h"
#include "Math/Color.h"

using namespace Enigma;

static FSceneView IdentityView()
{
	return FSceneView{};
}

// ---------------------------------------------------------------
// DrawCell
// ---------------------------------------------------------------

TEST(DrawCommandTest, Cell_SingleCell)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 2; cmd.WorldY = 3; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'@', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	EXPECT_EQ(buf.ReadCell(2, 3).Character, '@');
	EXPECT_EQ(buf.ReadCell(2, 3).Foreground, FColor::Red);
}

// ---------------------------------------------------------------
// DrawText
// ---------------------------------------------------------------

TEST(DrawCommandTest, Text_HorizontalString)
{
	FAsciiBackBuffer buf;
	buf.Allocate(20, 5);

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Text;
	cmd.WorldX = 1; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Text = "Hello";
	cmd.Fg = FColor::Green;
	cmd.Bg = FColor::Black;
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	EXPECT_EQ(buf.ReadCell(1, 0).Character, 'H');
	EXPECT_EQ(buf.ReadCell(2, 0).Character, 'e');
	EXPECT_EQ(buf.ReadCell(3, 0).Character, 'l');
	EXPECT_EQ(buf.ReadCell(4, 0).Character, 'l');
	EXPECT_EQ(buf.ReadCell(5, 0).Character, 'o');
	EXPECT_EQ(buf.ReadCell(1, 0).Foreground, FColor::Green);
}

TEST(DrawCommandTest, Text_EmptyString)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Text;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Text = "";
	cmd.Fg = FColor::White;
	cmd.Bg = FColor::Black;
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	// Empty text should not modify buffer.
	FAsciiCell defaultCell;
	EXPECT_EQ(buf.ReadCell(0, 0), defaultCell);
}

// ---------------------------------------------------------------
// DrawSprite
// ---------------------------------------------------------------

TEST(DrawCommandTest, Sprite_2x2Grid)
{
	FAsciiSprite sprite(2, 2);
	sprite.At(0, 0) = FAsciiCell{'A', FColor::Red, FColor::Black};
	sprite.At(1, 0) = FAsciiCell{'B', FColor::Green, FColor::Black};
	sprite.At(0, 1) = FAsciiCell{'C', FColor::Blue, FColor::Black};
	sprite.At(1, 1) = FAsciiCell{'D', FColor::Yellow, FColor::Black};

	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Sprite;
	cmd.WorldX = 3; cmd.WorldY = 2; cmd.ZOrder = 0;
	cmd.Sprite = &sprite;
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	EXPECT_EQ(buf.ReadCell(3, 2).Character, 'A');
	EXPECT_EQ(buf.ReadCell(4, 2).Character, 'B');
	EXPECT_EQ(buf.ReadCell(3, 3).Character, 'C');
	EXPECT_EQ(buf.ReadCell(4, 3).Character, 'D');
}

TEST(DrawCommandTest, Sprite_TransparentCellsSkipped)
{
	FAsciiSprite sprite(2, 1);
	sprite.At(0, 0) = FAsciiCell{'X', FColor::Red, FColor::Black};
	sprite.At(1, 0) = FAsciiCell{'\0', FColor::Green, FColor::Black}; // transparent

	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);
	buf.WriteCell(1, 0, FAsciiCell{'#', FColor::White, FColor::Blue});

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Sprite;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Sprite = &sprite;
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	EXPECT_EQ(buf.ReadCell(0, 0).Character, 'X');
	EXPECT_EQ(buf.ReadCell(1, 0).Character, '#'); // preserved
}

// ---------------------------------------------------------------
// FillRect
// ---------------------------------------------------------------

TEST(DrawCommandTest, FillRect_3x2)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::FillRect;
	cmd.WorldX = 1; cmd.WorldY = 1; cmd.ZOrder = 0;
	cmd.Width = 3; cmd.Height = 2;
	cmd.Cell = FAsciiCell{'#', FColor::Yellow, FColor::Black};
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	for (int y = 1; y <= 2; ++y)
	{
		for (int x = 1; x <= 3; ++x)
		{
			EXPECT_EQ(buf.ReadCell(x, y).Character, '#')
				<< "at (" << x << "," << y << ")";
		}
	}
}

// ---------------------------------------------------------------
// DrawBox (border only)
// ---------------------------------------------------------------

TEST(DrawCommandTest, DrawBox_4x3_BorderOnly)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::DrawBox;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Width = 4; cmd.Height = 3;
	cmd.Fg = FColor::White;
	cmd.Bg = FColor::Black;
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	// Corners: '+'
	EXPECT_EQ(buf.ReadCell(0, 0).Character, '+');
	EXPECT_EQ(buf.ReadCell(3, 0).Character, '+');
	EXPECT_EQ(buf.ReadCell(0, 2).Character, '+');
	EXPECT_EQ(buf.ReadCell(3, 2).Character, '+');

	// Top/bottom edges: '-'
	EXPECT_EQ(buf.ReadCell(1, 0).Character, '-');
	EXPECT_EQ(buf.ReadCell(2, 0).Character, '-');
	EXPECT_EQ(buf.ReadCell(1, 2).Character, '-');
	EXPECT_EQ(buf.ReadCell(2, 2).Character, '-');

	// Left/right edges: '|'
	EXPECT_EQ(buf.ReadCell(0, 1).Character, '|');
	EXPECT_EQ(buf.ReadCell(3, 1).Character, '|');

	// Interior should be untouched (default cell).
	FAsciiCell defaultCell;
	EXPECT_EQ(buf.ReadCell(1, 1), defaultCell);
	EXPECT_EQ(buf.ReadCell(2, 1), defaultCell);
}

TEST(DrawCommandTest, DrawBox_1x1_SingleCorner)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::DrawBox;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Width = 1; cmd.Height = 1;
	cmd.Fg = FColor::White;
	cmd.Bg = FColor::Black;
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(IdentityView(), buf);

	EXPECT_EQ(buf.ReadCell(0, 0).Character, '+');
}
