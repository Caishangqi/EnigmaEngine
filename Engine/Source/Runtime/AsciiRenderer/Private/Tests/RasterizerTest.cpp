// Copyright EnigmaEngine. All Rights Reserved.

/// @file RasterizerTest.cpp
/// @brief Unit tests for FRasterizer: draw list, stable depth sort,
///        transparency skip, viewport clipping, camera transform.

#include "AutomationTest/AutomationTest.h"

#define ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(SuiteName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST( \
        F##SuiteName##_##TestName##AutomationTest, \
        "System.AsciiRenderer." #SuiteName "." #TestName, \
        AsciiRenderer, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

#define ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST_F(FixtureName, TestName) \
    ENIGMA_IMPLEMENT_AUTOMATION_TEST_F( \
        FixtureName, \
        F##FixtureName##_##TestName##AutomationTest, \
        "System.AsciiRenderer." #FixtureName "." #TestName, \
        AsciiRenderer, \
        ::Enigma::EAutomationTestType::Integration, \
        ::Enigma::EAutomationTestFlags::RequiresEngine)

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

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, EmptyDrawList_NoChange)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);
	FAsciiCell fill{'#', FColor::Red, FColor::Blue};
	buf.Clear(fill);

	FRasterizer rasterizer;
	rasterizer.Rasterize(MakeView(), buf);

	// Buffer should be unchanged.
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0), fill);
	TestEqual("EXPECT_EQ", buf.ReadCell(4, 4), fill);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, SingleCommand_WritesToBuffer)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(3, 4, 0, 'A'));
	rasterizer.Rasterize(MakeView(), buf);

	TestEqual("EXPECT_EQ", buf.ReadCell(3, 4).Character, 'A');
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, Clear_ResetsDrawList)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'X'));
	rasterizer.Clear();
	rasterizer.Rasterize(MakeView(), buf);

	// After Clear, nothing should be drawn.
	FAsciiCell defaultCell;
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0), defaultCell);
}

// ---------------------------------------------------------------
// Stable depth sort (FIFO for equal Z)
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, DepthSort_HigherZOnTop)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(0, 0, 1, 'B')); // higher Z, drawn later
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'A')); // lower Z, drawn first
	rasterizer.Rasterize(MakeView(), buf);

	// Z=1 overwrites Z=0 (back-to-front).
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0).Character, 'B');
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, DepthSort_StableFIFO_EqualZ)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'A')); // submitted first
	rasterizer.AddCommand(MakeCellCmd(0, 0, 0, 'B')); // submitted second
	rasterizer.Rasterize(MakeView(), buf);

	// Stable sort: same Z, later submission overwrites earlier.
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0).Character, 'B');
}

// ---------------------------------------------------------------
// Transparency skip
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, TransparentCell_Skipped)
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
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0).Character, '#');
}

// ---------------------------------------------------------------
// Viewport clipping (frustum cull)
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, ViewportClip_OffScreenLeft)
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
		TestEqual("EXPECT_EQ", buf.ReadCell(x, 0), defaultCell);
	}
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, ViewportClip_OffScreenRight)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(5, 0, 0, 'X'));
	rasterizer.Rasterize(MakeView(), buf);

	FAsciiCell defaultCell;
	TestEqual("EXPECT_EQ", buf.ReadCell(4, 0), defaultCell);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, ViewportClip_OnEdge_Visible)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FRasterizer rasterizer;
	rasterizer.AddCommand(MakeCellCmd(4, 4, 0, 'E'));
	rasterizer.Rasterize(MakeView(), buf);

	TestEqual("EXPECT_EQ", buf.ReadCell(4, 4).Character, 'E');
}

// ---------------------------------------------------------------
// Camera transform offset
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, CameraOffset_ShiftsCommands)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FRasterizer rasterizer;
	// World position (5, 3), camera at (2, 1) -> screen (3, 2).
	rasterizer.AddCommand(MakeCellCmd(5, 3, 0, 'C'));
	rasterizer.Rasterize(MakeView(2.0f, 1.0f), buf);

	TestEqual("EXPECT_EQ", buf.ReadCell(3, 2).Character, 'C');
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(RasterizerTest, CameraOffset_CullsShiftedOffScreen)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);

	FRasterizer rasterizer;
	// World (1, 0), camera at (10, 0) -> screen (-9, 0) = off-screen.
	rasterizer.AddCommand(MakeCellCmd(1, 0, 0, 'X'));
	rasterizer.Rasterize(MakeView(10.0f, 0.0f), buf);

	FAsciiCell defaultCell;
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0), defaultCell);
}
