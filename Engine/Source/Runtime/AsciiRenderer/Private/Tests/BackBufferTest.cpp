// Copyright EnigmaEngine. All Rights Reserved.

/// @file BackBufferTest.cpp
/// @brief Unit tests for FAsciiBackBuffer: allocate, clear, write/read, resize.

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

#include "AsciiBackBuffer.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

using namespace Enigma;

// ---------------------------------------------------------------
// Allocate
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Allocate_Dimensions)
{
	FAsciiBackBuffer buf;
	buf.Allocate(80, 25);
	TestEqual("EXPECT_EQ", buf.GetWidth(), 80);
	TestEqual("EXPECT_EQ", buf.GetHeight(), 25);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Allocate_DefaultCells)
{
	FAsciiBackBuffer buf;
	buf.Allocate(4, 3);
	FAsciiCell defaultCell;
	for (int y = 0; y < 3; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			TestEqual("EXPECT_EQ", buf.ReadCell(x, y), defaultCell);
		}
	}
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Allocate_ZeroDimensions)
{
	FAsciiBackBuffer buf;
	buf.Allocate(0, 0);
	TestEqual("EXPECT_EQ", buf.GetWidth(), 0);
	TestEqual("EXPECT_EQ", buf.GetHeight(), 0);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Allocate_NegativeDimensions)
{
	FAsciiBackBuffer buf;
	buf.Allocate(-5, -3);
	TestEqual("EXPECT_EQ", buf.GetWidth(), 0);
	TestEqual("EXPECT_EQ", buf.GetHeight(), 0);
}

// ---------------------------------------------------------------
// Clear
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Clear_DefaultCell)
{
	FAsciiBackBuffer buf;
	buf.Allocate(3, 3);
	buf.WriteCell(1, 1, FAsciiCell{'X', FColor::Red, FColor::Blue});
	buf.Clear();

	FAsciiCell expected{' ', FColor::White, FColor::Black};
	TestEqual("EXPECT_EQ", buf.ReadCell(1, 1), expected);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Clear_CustomCell)
{
	FAsciiBackBuffer buf;
	buf.Allocate(2, 2);
	FAsciiCell fill{'#', FColor::Green, FColor::Yellow};
	buf.Clear(fill);

	for (int y = 0; y < 2; ++y)
	{
		for (int x = 0; x < 2; ++x)
		{
			TestEqual("EXPECT_EQ", buf.ReadCell(x, y), fill);
		}
	}
}

// ---------------------------------------------------------------
// WriteCell / ReadCell
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, WriteRead_ValidPosition)
{
	FAsciiBackBuffer buf;
	buf.Allocate(10, 10);
	FAsciiCell cell{'@', FColor::Red, FColor::Green};
	buf.WriteCell(5, 7, cell);
	TestEqual("EXPECT_EQ", buf.ReadCell(5, 7), cell);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Write_OutOfBounds_Ignored)
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

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Read_OutOfBounds_ReturnsDefault)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);
	FAsciiCell defaultCell;
	TestEqual("EXPECT_EQ", buf.ReadCell(-1, 0), defaultCell);
	TestEqual("EXPECT_EQ", buf.ReadCell(0, -1), defaultCell);
	TestEqual("EXPECT_EQ", buf.ReadCell(5, 0), defaultCell);
	TestEqual("EXPECT_EQ", buf.ReadCell(0, 5), defaultCell);
}

// ---------------------------------------------------------------
// Resize (re-allocate)
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, Resize_ClearsOldData)
{
	FAsciiBackBuffer buf;
	buf.Allocate(5, 5);
	buf.WriteCell(2, 2, FAsciiCell{'X', FColor::Red, FColor::Blue});

	buf.Allocate(10, 10);
	FAsciiCell defaultCell;
	TestEqual("EXPECT_EQ", buf.ReadCell(2, 2), defaultCell);
	TestEqual("EXPECT_EQ", buf.GetWidth(), 10);
	TestEqual("EXPECT_EQ", buf.GetHeight(), 10);
}

// ---------------------------------------------------------------
// GetData
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, GetData_NonNull)
{
	FAsciiBackBuffer buf;
	buf.Allocate(3, 3);
	TestNotEqual("EXPECT_NE", buf.GetData(), nullptr);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(BackBufferTest, GetData_RowMajorLayout)
{
	FAsciiBackBuffer buf;
	buf.Allocate(3, 2);
	buf.WriteCell(0, 0, FAsciiCell{'A', FColor::White, FColor::Black});
	buf.WriteCell(2, 1, FAsciiCell{'F', FColor::White, FColor::Black});

	const FAsciiCell* data = buf.GetData();
	TestEqual("EXPECT_EQ", data[0].Character, 'A'); // (0,0) = index 0
	TestEqual("EXPECT_EQ", data[5].Character, 'F'); // (2,1) = index 1*3+2 = 5
}
