// Copyright EnigmaEngine. All Rights Reserved.

/// @file RasterizerTest.cpp
/// @brief Unit tests for FRasterizer: draw list, stable depth sort,
///        transparency skip, viewport clipping, camera transform.

#include <gtest/gtest.h>

#include "AsciiRasterizer.h"
#include "AsciiBackBuffer.h"
#include "RenderCore/AsciiCell.h"
#include "RenderCore/AsciiBlendState.h"
#include "SceneView/SceneView.h"
#include "Math/Color.h"

using namespace Enigma;

static FSceneView MakeView(float camX = 0.0f, float camY = 0.0f)
{
	FSceneView view;
	view.ViewTransform = FTransform(FVector(camX, camY, 0.0f));
	return view;
}

static FDrawCommand MakeCellCmd(int32_t x, int32_t y, int32_t z, char ch)
{
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = x; cmd.WorldY = y; cmd.ZOrder = z;
	cmd.Cell = FAsciiCell{ch, FColor::White, FColor::Black};
	cmd.BlendState = FAsciiBlendState::Opaque();
	return cmd;
}

// ---------------------------------------------------------------
// Draw list accumulation
// ---------------------------------------------------------------

TEST(RasterizerTest, EmptyDrawList_NoChange)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);
	FAsciiCell fill{'#', FColor::Red, FColor::Blue};
	buf.Clear(fill);

	FRasterizer rasterizer;
	rasterizer.Rasterize(MakeView(), buf);

	// Buffer should be unchanged.
	EXPECT_EQ(buf.ReadCell(0, 0), fill);
	EXPECT_EQ(buf.ReadCell(4, 4), fill);
}

TEST(RasterizerTest, SingleCommand_WritesToBuffer)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(3, 4, 0, 'A'));
	rasterizer.Rasterize(MakeView(), buf);

	EXPECT_EQ(buf.ReadCell(3, 4).Character, 'A');
}

TEST(RasterizerTest, Clear_ResetsDrawList)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'X'));
	rasterizer.Clear();
	rasterizer.Rasterize(MakeView(), buf);

	// After Clear, nothing should be drawn.
	FAsciiCell defaultCell;
	EXPECT_EQ(buf.ReadCell(0, 0), defaultCell);
}

// ---------------------------------------------------------------
// Stable depth sort (FIFO for equal Z)
// ---------------------------------------------------------------

TEST(RasterizerTest, DepthSort_HigherZOnTop)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(0, 0, 1, 'B')); // higher Z, drawn later
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'A')); // lower Z, drawn first
	rasterizer.Rasterize(MakeView(), buf);

	// Z=1 overwrites Z=0 (back-to-front).
	EXPECT_EQ(buf.ReadCell(0, 0).Character, 'B');
}

TEST(RasterizerTest, DepthSort_StableFIFO_EqualZ)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'A')); // submitted first
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'B')); // submitted second
	rasterizer.Rasterize(MakeView(), buf);

	// Stable sort: same Z, later submission overwrites earlier.
	EXPECT_EQ(buf.ReadCell(0, 0).Character, 'B');
}

// ---------------------------------------------------------------
// Transparency skip
// ---------------------------------------------------------------

TEST(RasterizerTest, TransparentCell_Skipped)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);
	buf.WriteCell(0, 0, FAsciiCell{'#', FColor::Red, FColor::Blue});

	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 1;
	cmd.Cell = FAsciiCell{'\0', FColor::Green, FColor::Yellow}; // transparent
	cmd.BlendState = FAsciiBlendState::Opaque();

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(MakeView(), buf);

	// Transparent cell should not overwrite.
	EXPECT_EQ(buf.ReadCell(0, 0).Character, '#');
}

// ---------------------------------------------------------------
// Viewport clipping (frustum cull)
// ---------------------------------------------------------------

TEST(RasterizerTest, ViewportClip_OffScreenLeft)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(-1, 0, 0, 'X'));
	rasterizer.Rasterize(MakeView(), buf);

	// Off-screen command should be culled, buffer unchanged.
	FAsciiCell defaultCell;
	for (int x = 0; x < 5; ++x)
	{
		EXPECT_EQ(buf.ReadCell(x, 0), defaultCell);
	}
}

TEST(RasterizerTest, ViewportClip_OffScreenRight)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(5, 0, 0, 'X'));
	rasterizer.Rasterize(MakeView(), buf);

	FAsciiCell defaultCell;
	EXPECT_EQ(buf.ReadCell(4, 0), defaultCell);
}

TEST(RasterizerTest, ViewportClip_OnEdge_Visible)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(4, 4, 0, 'E'));
	rasterizer.Rasterize(MakeView(), buf);

	EXPECT_EQ(buf.ReadCell(4, 4).Character, 'E');
}

// ---------------------------------------------------------------
// Camera transform offset
// ---------------------------------------------------------------

TEST(RasterizerTest, CameraOffset_ShiftsCommands)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	// World position (5, 3), camera at (2, 1) -> screen (3, 2).
	rasterizer.AddCommand(MakeCellCmd(5, 3, 0, 'C'));
	rasterizer.Rasterize(MakeView(2.0f, 1.0f), buf);

	EXPECT_EQ(buf.ReadCell(3, 2).Character, 'C');
}

TEST(RasterizerTest, CameraOffset_CullsShiftedOffScreen)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FRasterizer rasterizer;
	// World (1, 0), camera at (10, 0) -> screen (-9, 0) = off-screen.
	rasterizer.AddCommand(MakeCellCmd(1, 0, 0, 'X'));
	rasterizer.Rasterize(MakeView(10.0f, 0.0f), buf);

	FAsciiCell defaultCell;
	EXPECT_EQ(buf.ReadCell(0, 0), defaultCell);
}
