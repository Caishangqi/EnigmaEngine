// Copyright EnigmaEngine. All Rights Reserved.

/// @file BlendStateTest.cpp
/// @brief Unit tests for FAsciiBlendState: presets, blend ops, write mask,
///        chainable setters, operator==, and rasterizer blend integration.

#include <gtest/gtest.h>

#include "AsciiRasterizer.h"
#include "AsciiBackBuffer.h"
#include "RenderCore/AsciiBlendState.h"
#include "RenderCore/AsciiCell.h"
#include "RenderCore/BlendState.h"
#include "SceneView/SceneView.h"
#include "Math/Color.h"

using namespace Enigma;

/// Helper: create an identity SceneView (camera at origin).
static FSceneView MakeIdentityView()
{
	return FSceneView{};
}

/// Helper: rasterize a single command into a buffer and return the result cell.
static FAsciiCell RasterizeOne(
	FAsciiCell dstCell, FDrawCommand cmd, int32_t bufW = 10, int32_t bufH = 10)
{
	FAsciiBackBuffer buf;
	buf.Allocate(bufW, bufH);
	buf.WriteCell(cmd.WorldX, cmd.WorldY, dstCell);

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd));
	rasterizer.Rasterize(MakeIdentityView(), buf);

	return buf.ReadCell(cmd.WorldX, cmd.WorldY);
}

// ---------------------------------------------------------------
// 1. Opaque = no-blend (Replace all channels)
// ---------------------------------------------------------------

TEST(BlendStateTest, Opaque_ReplacesAll)
{
	FAsciiCell dst{'#', FColor(100, 100, 100), FColor(50, 50, 50)};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'@', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState::Opaque();

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Character, '@');
	EXPECT_EQ(result.Foreground, FColor::Red);
	EXPECT_EQ(result.Background, FColor::Blue);
}

// ---------------------------------------------------------------
// 2. Add clamp: FColor(200,100,50) + FColor(100,200,250) = FColor(255,255,255)
// ---------------------------------------------------------------

TEST(BlendStateTest, Add_ClampsTo255)
{
	FAsciiCell dst{' ', FColor(100, 200, 250), FColor(100, 200, 250)};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'X', FColor(200, 100, 50), FColor(200, 100, 50)};
	cmd.BlendState = FAsciiBlendState{};
	cmd.BlendState.ForegroundOp = EBlendOp::Add;
	cmd.BlendState.BackgroundOp = EBlendOp::Add;

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Foreground, FColor(255, 255, 255));
	EXPECT_EQ(result.Background, FColor(255, 255, 255));
}

// ---------------------------------------------------------------
// 3. Multiply: FColor(128,255,0) * FColor(255,128,255) = FColor(128,128,0)
// ---------------------------------------------------------------

TEST(BlendStateTest, Multiply_ScalesCorrectly)
{
	FAsciiCell dst{' ', FColor(255, 128, 255), FColor(255, 128, 255)};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'X', FColor(128, 255, 0), FColor(128, 255, 0)};
	cmd.BlendState = FAsciiBlendState{};
	cmd.BlendState.ForegroundOp = EBlendOp::Multiply;
	cmd.BlendState.BackgroundOp = EBlendOp::Multiply;

	FAsciiCell result = RasterizeOne(dst, cmd);
	// (128*255)/255=128, (255*128)/255=128, (0*255)/255=0
	EXPECT_EQ(result.Foreground, FColor(128, 128, 0));
	EXPECT_EQ(result.Background, FColor(128, 128, 0));
}

// ---------------------------------------------------------------
// 4. KeepNonSpace: preserves existing non-space '@' over '#'
// ---------------------------------------------------------------

TEST(BlendStateTest, KeepNonSpace_PreservesExisting)
{
	FAsciiCell dst{'@', FColor::White, FColor::Black};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'#', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState::Transparent(); // KeepNonSpace

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Character, '@'); // dst non-space preserved
}

TEST(BlendStateTest, KeepNonSpace_OverwritesSpace)
{
	FAsciiCell dst{' ', FColor::White, FColor::Black};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'#', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState::Transparent();

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Character, '#'); // dst was space, overwritten
}

// ---------------------------------------------------------------
// 5. Custom lambda blend
// ---------------------------------------------------------------

TEST(BlendStateTest, CustomLambda_Foreground)
{
	FAsciiCell dst{' ', FColor(10, 20, 30), FColor::Black};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'X', FColor(100, 100, 100), FColor::Black};
	cmd.BlendState = FAsciiBlendState{};
	cmd.BlendState.WithCustomForeground(
		[](FColor src, FColor dst) -> FColor
		{
			// Average
			return FColor(
				static_cast<uint8_t>((src.R + dst.R) / 2),
				static_cast<uint8_t>((src.G + dst.G) / 2),
				static_cast<uint8_t>((src.B + dst.B) / 2));
		});

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Foreground, FColor(55, 60, 65));
}

// ---------------------------------------------------------------
// 6. Per-command blend: overlapping commands with different states
// ---------------------------------------------------------------

TEST(BlendStateTest, PerCommandBlend_DifferentStates)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);

	FDrawCommand cmd1;
	cmd1.Type = EDrawCommandType::Cell;
	cmd1.WorldX = 0; cmd1.WorldY = 0; cmd1.ZOrder = 0;
	cmd1.Cell = FAsciiCell{'A', FColor(100, 0, 0), FColor::Black};
	cmd1.BlendState = FAsciiBlendState::Opaque();

	FDrawCommand cmd2;
	cmd2.Type = EDrawCommandType::Cell;
	cmd2.WorldX = 0; cmd2.WorldY = 0; cmd2.ZOrder = 1;
	cmd2.Cell = FAsciiCell{'B', FColor(50, 0, 0), FColor::Black};
	cmd2.BlendState = FAsciiBlendState{};
	cmd2.BlendState.ForegroundOp = EBlendOp::Add;

	FRasterizer rasterizer;
	rasterizer.AddCommand(std::move(cmd1));
	rasterizer.AddCommand(std::move(cmd2));
	rasterizer.Rasterize(MakeIdentityView(), buf);

	FAsciiCell result = buf.ReadCell(0, 0);
	// cmd1 writes FColor(100,0,0), cmd2 adds FColor(50,0,0) -> FColor(150,0,0)
	EXPECT_EQ(result.Foreground, FColor(150, 0, 0));
}

// ---------------------------------------------------------------
// 7. Write mask: bWriteCharacter = false preserves dst character
// ---------------------------------------------------------------

TEST(BlendStateTest, WriteMask_CharFalse_PreservesChar)
{
	FAsciiCell dst{'@', FColor::White, FColor::Black};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'X', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState{};
	cmd.BlendState.WithWriteMask(false, true, true);

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Character, '@');       // preserved
	EXPECT_EQ(result.Foreground, FColor::Red);  // written
	EXPECT_EQ(result.Background, FColor::Blue); // written
}

// ---------------------------------------------------------------
// 8. Write mask: bWriteForeground=false, bWriteBackground=false
// ---------------------------------------------------------------

TEST(BlendStateTest, WriteMask_FgBgFalse_PreservesColors)
{
	FAsciiCell dst{'@', FColor(10, 20, 30), FColor(40, 50, 60)};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'X', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState{};
	cmd.BlendState.WithWriteMask(true, false, false);

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Character, 'X');              // written
	EXPECT_EQ(result.Foreground, FColor(10, 20, 30)); // preserved
	EXPECT_EQ(result.Background, FColor(40, 50, 60)); // preserved
}

// ---------------------------------------------------------------
// 9. Write mask: all three false = no change
// ---------------------------------------------------------------

TEST(BlendStateTest, WriteMask_AllFalse_NoChange)
{
	FAsciiCell dst{'@', FColor(10, 20, 30), FColor(40, 50, 60)};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'X', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState{};
	cmd.BlendState.WithWriteMask(false, false, false);

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result, dst);
}

// ---------------------------------------------------------------
// 10. Chainable setters: Transparent().WithForeground(Add)
// ---------------------------------------------------------------

TEST(BlendStateTest, Chainable_TransparentWithAddFg)
{
	auto state = FAsciiBlendState::Transparent().WithForeground(EBlendOp::Add);
	EXPECT_EQ(state.CharacterOp, ECharBlendOp::KeepNonSpace);
	EXPECT_EQ(state.ForegroundOp, EBlendOp::Add);
	EXPECT_EQ(state.BackgroundOp, EBlendOp::Replace);
}

// ---------------------------------------------------------------
// 11. Chainable setters: Opaque().WithWriteMask(false, false, true)
//     writes background only
// ---------------------------------------------------------------

TEST(BlendStateTest, Chainable_OpaqueWriteBgOnly)
{
	FAsciiCell dst{'@', FColor(10, 20, 30), FColor(40, 50, 60)};
	FDrawCommand cmd;
	cmd.Type = EDrawCommandType::Cell;
	cmd.WorldX = 0; cmd.WorldY = 0; cmd.ZOrder = 0;
	cmd.Cell = FAsciiCell{'X', FColor::Red, FColor::Blue};
	cmd.BlendState = FAsciiBlendState::Opaque().WithWriteMask(false, false, true);

	FAsciiCell result = RasterizeOne(dst, cmd);
	EXPECT_EQ(result.Character, '@');              // preserved
	EXPECT_EQ(result.Foreground, FColor(10, 20, 30)); // preserved
	EXPECT_EQ(result.Background, FColor::Blue);        // written
}

// ---------------------------------------------------------------
// 12. operator== correctness
// ---------------------------------------------------------------

TEST(BlendStateTest, Equality_SamePresets)
{
	EXPECT_EQ(FAsciiBlendState::Opaque(), FAsciiBlendState::Opaque());
	EXPECT_EQ(FAsciiBlendState::Transparent(), FAsciiBlendState::Transparent());
	EXPECT_EQ(FAsciiBlendState::Additive(), FAsciiBlendState::Additive());
}

TEST(BlendStateTest, Equality_DifferentPresets)
{
	EXPECT_NE(FAsciiBlendState::Opaque(), FAsciiBlendState::Transparent());
	EXPECT_NE(FAsciiBlendState::Opaque(), FAsciiBlendState::Additive());
}

TEST(BlendStateTest, Equality_WriteMaskDifference)
{
	auto a = FAsciiBlendState::Opaque();
	auto b = FAsciiBlendState::Opaque().WithWriteMask(false, true, true);
	EXPECT_NE(a, b);
}

TEST(BlendStateTest, Equality_CustomFnNullptrState)
{
	auto a = FAsciiBlendState::Opaque();
	auto b = FAsciiBlendState::Opaque();
	b.WithCustomForeground([](FColor s, FColor) { return s; });
	// a has no custom fn (nullptr), b has one (non-nullptr)
	EXPECT_NE(a, b);
}

TEST(BlendStateTest, Equality_BothCustomFnNonNull)
{
	auto a = FAsciiBlendState::Opaque();
	a.WithCustomForeground([](FColor s, FColor) { return s; });
	auto b = FAsciiBlendState::Opaque();
	b.WithCustomForeground([](FColor, FColor d) { return d; });
	// Both have non-null custom fn -> considered equal by nullptr state
	EXPECT_EQ(a, b);
}
