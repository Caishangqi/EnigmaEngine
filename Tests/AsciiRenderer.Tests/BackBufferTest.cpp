// Copyright EnigmaEngine. All Rights Reserved.

/// @file BackBufferTest.cpp
/// @brief Unit tests for FAsciiBackBuffer: allocate, clear, write/read, resize.

#include <gtest/gtest.h>

#include "AsciiBackBuffer.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

using namespace Enigma;

// ---------------------------------------------------------------
// Allocate
// ---------------------------------------------------------------

TEST(BackBufferTest, Allocate_Dimensions)
{
	FAsciiBackBuffer buf;
	buf.Allocate(80, 25);
	EXPECT_EQ(buf.GetWidth(), 80);
	EXPECT_EQ(buf.GetHeight(), 25);
}

TEST(BackBufferTest, Allocate_DefaultCells)
{
	FAsciiBackBuffer buf;
	buf.Allocate(4, 3);
	FAsciiCell defaultCell;
	for (int y = 0; y < 3; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			EXPECT_EQ(buf.ReadCell(x, y), defaultCell);
		}
	}
}

TEST(BackBufferTest, Allocate_ZeroDimensions)
{
	FAsciiBackBuffer buf;
	buf.Allocate(0, 0);
	EXPECT_EQ(buf.GetWidth(), 0);
	EXPECT_EQ(buf.GetHeight(), 0);
}

TEST(BackBufferTest, Allocate_NegativeDimensions)
{
	FAsciiBackBuffer buf;
	buf.Allocate(-5, -3);
	EXPECT_EQ(buf.GetWidth(), 0);
	EXPECT_EQ(buf.GetHeight(), 0);
}

// ---------------------------------------------------------------
// Clear
// ---------------------------------------------------------------

TEST(BackBufferTest, Clear_DefaultCell)
{
	FAsciiBackBuffer buf;
	buf.Allocate(3, 3);
	buf.WriteCell(1, 1, FAsciiCell{'X', FColor::Red, FColor::Blue});
	buf.Clear();

	FAsciiCell expected{' ', FColor::White, FColor::Black};
	EXPECT_EQ(buf.ReadCell(1, 1), expected);
}

TEST(BackBufferTest, Clear_CustomCell)
{
	FAsciiBackBuffer buf;
	buf.Allocate(2, 2);
	FAsciiCell fill{'#', FColor::Green, FColor::Yellow};
	buf.Clear(fill);

	for (int y = 0; y < 2; ++y)
	{
		for (int x = 0; x < 2; ++x)
		{
			EXPECT_EQ(buf.ReadCell(x, y), fill);
		}
	}
}

// ---------------------------------------------------------------
// WriteCell / ReadCell
// ---------------------------------------------------------------

TEST(BackBufferTest, WriteRead_ValidPosition)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);
	FAsciiCell cell{'@', FColor::Red, FColor::Green};
	buf.WriteCell(5, 7, cell);
	EXPECT_EQ(buf.ReadCell(5, 7), cell);
}

TEST(BackBufferTest, Write_OutOfBounds_Ignored)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);
	FAsciiCell cell{'X', FColor::Red, FColor::Blue};
	// These should not crash.
	buf.WriteCell(-1, 0, cell);
	buf.WriteCell(0, -1, cell);
	buf.WriteCell(5, 0, cell);
	buf.WriteCell(0, 5, cell);
	buf.WriteCell(100, 100, cell);
}

TEST(BackBufferTest, Read_OutOfBounds_ReturnsDefault)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);
	FAsciiCell defaultCell;
	EXPECT_EQ(buf.ReadCell(-1, 0), defaultCell);
	EXPECT_EQ(buf.ReadCell(0, -1), defaultCell);
	EXPECT_EQ(buf.ReadCell(5, 0), defaultCell);
	EXPECT_EQ(buf.ReadCell(0, 5), defaultCell);
}

// ---------------------------------------------------------------
// Resize (re-allocate)
// ---------------------------------------------------------------

TEST(BackBufferTest, Resize_ClearsOldData)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);
	buf.WriteCell(2, 2, FAsciiCell{'X', FColor::Red, FColor::Blue});

	buf.Allocate(10, 10);
	FAsciiCell defaultCell;
	EXPECT_EQ(buf.ReadCell(2, 2), defaultCell);
	EXPECT_EQ(buf.GetWidth(), 10);
	EXPECT_EQ(buf.GetHeight(), 10);
}

// ---------------------------------------------------------------
// GetData
// ---------------------------------------------------------------

TEST(BackBufferTest, GetData_NonNull)
{
	FAsciiBackBuffer buf;
	buf.Allocate(3, 3);
	EXPECT_NE(buf.GetData(), nullptr);
}

TEST(BackBufferTest, GetData_RowMajorLayout)
{
	FAsciiBackBuffer buf;
	buf.Allocate(3, 2);
	buf.WriteCell(0, 0, FAsciiCell{'A', FColor::White, FColor::Black});
	buf.WriteCell(2, 1, FAsciiCell{'F', FColor::White, FColor::Black});

	const FAsciiCell* data = buf.GetData();
	EXPECT_EQ(data[0].Character, 'A');       // (0,0) = index 0
	EXPECT_EQ(data[5].Character, 'F');       // (2,1) = index 1*3+2 = 5
}
