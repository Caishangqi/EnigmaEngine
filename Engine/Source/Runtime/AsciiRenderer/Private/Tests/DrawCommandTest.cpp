// Copyright EnigmaEngine. All Rights Reserved.

/// @file DrawCommandTest.cpp
/// @brief Unit tests for draw command rasterization: Cell, Text, Sprite,
///        FillRect, DrawBox.

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

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, Cell_SingleCell)
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

	TestEqual("EXPECT_EQ", buf.ReadCell(2, 3).Character, '@');
	TestEqual("EXPECT_EQ", buf.ReadCell(2, 3).Foreground, FColor::Red);
}

// ---------------------------------------------------------------
// DrawText
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, Text_HorizontalString)
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

	TestEqual("EXPECT_EQ", buf.ReadCell(1, 0).Character, 'H');
	TestEqual("EXPECT_EQ", buf.ReadCell(2, 0).Character, 'e');
	TestEqual("EXPECT_EQ", buf.ReadCell(3, 0).Character, 'l');
	TestEqual("EXPECT_EQ", buf.ReadCell(4, 0).Character, 'l');
	TestEqual("EXPECT_EQ", buf.ReadCell(5, 0).Character, 'o');
	TestEqual("EXPECT_EQ", buf.ReadCell(1, 0).Foreground, FColor::Green);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, Text_EmptyString)
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
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0), defaultCell);
}

// ---------------------------------------------------------------
// DrawSprite
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, Sprite_2x2Grid)
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

	TestEqual("EXPECT_EQ", buf.ReadCell(3, 2).Character, 'A');
	TestEqual("EXPECT_EQ", buf.ReadCell(4, 2).Character, 'B');
	TestEqual("EXPECT_EQ", buf.ReadCell(3, 3).Character, 'C');
	TestEqual("EXPECT_EQ", buf.ReadCell(4, 3).Character, 'D');
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, Sprite_TransparentCellsSkipped)
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

	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0).Character, 'X');
	TestEqual("EXPECT_EQ", buf.ReadCell(1, 0).Character, '#'); // preserved
}

// ---------------------------------------------------------------
// FillRect
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, FillRect_3x2)
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
			TestEqual("EXPECT_EQ", buf.ReadCell(x, y).Character, '#');
		}
	}
}

// ---------------------------------------------------------------
// DrawBox (border only)
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, DrawBox_4x3_BorderOnly)
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
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0).Character, '+');
	TestEqual("EXPECT_EQ", buf.ReadCell(3, 0).Character, '+');
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 2).Character, '+');
	TestEqual("EXPECT_EQ", buf.ReadCell(3, 2).Character, '+');

	// Top/bottom edges: '-'
	TestEqual("EXPECT_EQ", buf.ReadCell(1, 0).Character, '-');
	TestEqual("EXPECT_EQ", buf.ReadCell(2, 0).Character, '-');
	TestEqual("EXPECT_EQ", buf.ReadCell(1, 2).Character, '-');
	TestEqual("EXPECT_EQ", buf.ReadCell(2, 2).Character, '-');

	// Left/right edges: '|'
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 1).Character, '|');
	TestEqual("EXPECT_EQ", buf.ReadCell(3, 1).Character, '|');

	// Interior should be untouched (default cell).
	FAsciiCell defaultCell;
	TestEqual("EXPECT_EQ", buf.ReadCell(1, 1), defaultCell);
	TestEqual("EXPECT_EQ", buf.ReadCell(2, 1), defaultCell);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(DrawCommandTest, DrawBox_1x1_SingleCorner)
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

	TestEqual("EXPECT_EQ", buf.ReadCell(0, 0).Character, '+');
}
