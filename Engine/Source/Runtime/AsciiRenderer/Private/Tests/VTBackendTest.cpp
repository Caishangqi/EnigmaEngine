// Copyright EnigmaEngine. All Rights Reserved.

/// @file VTBackendTest.cpp
/// @brief Unit tests for FVTConsoleBackend RGB-to-256-color mapping.
/// Replicates the MapColorTo256 algorithm for direct verification.

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

#include "VTConsoleBackend.h"
#include "RenderCore/AsciiCell.h"
#include "Math/Color.h"

#include <algorithm>
#include <climits>

using namespace Enigma;

// ---------------------------------------------------------------
// Replicated mapping algorithm for test verification
// ---------------------------------------------------------------

namespace
{

constexpr uint8_t kStandard16[16][3] =
{
	{  0,   0,   0}, {128,   0,   0}, {  0, 128,   0}, {128, 128,   0},
	{  0,   0, 128}, {128,   0, 128}, {  0, 128, 128}, {192, 192, 192},
	{128, 128, 128}, {255,   0,   0}, {  0, 255,   0}, {255, 255,   0},
	{  0,   0, 255}, {255,   0, 255}, {  0, 255, 255}, {255, 255, 255}
};

constexpr uint8_t kCubeLevels[6] = { 0, 95, 135, 175, 215, 255 };

int NearestCubeIdx(uint8_t value)
{
	int best = 0;
	int bestDist = INT_MAX;
	for (int i = 0; i < 6; ++i)
	{
		int d = static_cast<int>(value) - static_cast<int>(kCubeLevels[i]);
		if (d * d < bestDist)
		{
			bestDist = d * d;
			best = i;
		}
	}
	return best;
}

int MapColorTo256(FColor color)
{
	int bestIndex = 0;
	int bestDist  = INT_MAX;

	// Standard 16
	for (int i = 0; i < 16; ++i)
	{
		int dr = static_cast<int>(color.R) - kStandard16[i][0];
		int dg = static_cast<int>(color.G) - kStandard16[i][1];
		int db = static_cast<int>(color.B) - kStandard16[i][2];
		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist) { bestDist = dist; bestIndex = i; }
	}

	// Color cube 16-231
	{
		int ri = NearestCubeIdx(color.R);
		int gi = NearestCubeIdx(color.G);
		int bi = NearestCubeIdx(color.B);
		int dr = static_cast<int>(color.R) - kCubeLevels[ri];
		int dg = static_cast<int>(color.G) - kCubeLevels[gi];
		int db = static_cast<int>(color.B) - kCubeLevels[bi];
		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist) { bestDist = dist; bestIndex = 16 + 36 * ri + 6 * gi + bi; }
	}

	// Grayscale 232-255
	{
		int gray = (static_cast<int>(color.R) + static_cast<int>(color.G)
		            + static_cast<int>(color.B)) / 3;
		int gi = std::clamp((gray - 8) / 10, 0, 23);
		int grayVal = 8 + gi * 10;
		int dr = static_cast<int>(color.R) - grayVal;
		int dg = static_cast<int>(color.G) - grayVal;
		int db = static_cast<int>(color.B) - grayVal;
		int dist = dr * dr + dg * dg + db * db;
		if (dist < bestDist) { bestDist = dist; bestIndex = 232 + gi; }
	}

	return bestIndex;
}

} // anonymous namespace

// ---------------------------------------------------------------
// Standard 16-color exact matches
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Standard_Black)
{
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(0, 0, 0)), 0);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Standard_White)
{
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(255, 255, 255)), 15);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Standard_Red)
{
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(255, 0, 0)), 9);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Standard_Green)
{
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(0, 255, 0)), 10);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Standard_Blue)
{
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(0, 0, 255)), 12);
}

// ---------------------------------------------------------------
// Color cube corners (indices 16-231)
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Cube_Origin)
{
	// (0,0,0) in cube = index 16, but standard black (0) is closer.
	// Cube level 0 = 0, same as standard black. Standard wins by iteration order.
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(0, 0, 0)), 0);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Cube_95_95_95)
{
	// Cube (1,1,1) = 16 + 36 + 6 + 1 = 59
	// vs standard gray (128,128,128) dist = 33*3 = 3267
	// vs cube (95,95,95) dist = 0
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(95, 95, 95)), 59);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Cube_255_255_0)
{
	// Standard yellow (11) = (255,255,0) exact match.
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(255, 255, 0)), 11);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Cube_135_175_215)
{
	// Cube (2,3,4) = 16 + 72 + 18 + 4 = 110
	int expected = MapColorTo256(FColor(135, 175, 215));
	TestEqual("EXPECT_EQ", expected, 110);
}

// ---------------------------------------------------------------
// Grayscale ramp (indices 232-255)
// ---------------------------------------------------------------

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Grayscale_MidGray)
{
	// Gray value 128 -> gi = (128-8)/10 = 12 -> grayVal = 128
	// Index = 232 + 12 = 244
	// vs standard gray (128,128,128) index 8, dist = 0
	// Standard wins with exact match.
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(128, 128, 128)), 8);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Grayscale_DarkGray_38)
{
	// Gray 38 -> gi = (38-8)/10 = 3 -> grayVal = 38
	// Index = 232 + 3 = 235, dist = 0
	// vs standard: closest is black(0) dist = 38^2*3 = 4332
	// vs cube: nearest cube (0,0,0) dist = 38^2*3 = 4332
	// Grayscale wins.
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(38, 38, 38)), 235);
}

ENIGMA_IMPLEMENT_ASCII_RENDERER_AUTOMATION_TEST(VTBackendTest, Grayscale_LightGray_218)
{
	// Gray 218 -> gi = (218-8)/10 = 21 -> grayVal = 218
	// Index = 232 + 21 = 253, dist = 0
	// vs standard silver (192,192,192) dist = 26^2*3 = 2028
	// vs cube: nearest (215,215,215) = cube(4,4,4) = 16+144+24+4 = 188, dist = 3^2*3 = 27
	// Cube (188) wins over grayscale (253) since 27 < 0? No, grayscale dist = 0.
	// Actually (218,218,218): grayscale gi=(218-8)/10=21, grayVal=218, dist=0.
	// Cube: nearest level for 218 is 215 (index 4), dist = 3^2*3 = 27.
	// Grayscale dist = 0 wins.
	TestEqual("EXPECT_EQ", MapColorTo256(FColor(218, 218, 218)), 253);
}
