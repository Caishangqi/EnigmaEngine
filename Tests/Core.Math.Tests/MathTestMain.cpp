/// @file MathTestMain.cpp
/// @brief GoogleTest entry point for Core Math unit tests.
///
/// This file provides the main() function for the test executable.
/// Individual test files (FMathTest.cpp, FVectorTest.cpp, etc.) are
/// compiled alongside this file and auto-registered by GoogleTest.

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
