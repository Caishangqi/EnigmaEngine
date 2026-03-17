// Copyright EnigmaEngine. All Rights Reserved.

#include "Misc/Name.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using namespace Enigma;

// ---------------------------------------------------------------
// Default Construction
// ---------------------------------------------------------------

TEST(FNameTest, DefaultConstructionIsNone)
{
	FName name;
	EXPECT_TRUE(name.IsNone());
	EXPECT_EQ(name.GetIndex(), 0u);
}

TEST(FNameTest, DefaultEqualsNAME_None)
{
	FName name;
	EXPECT_EQ(name, NAME_None);
}

// ---------------------------------------------------------------
// String Construction
// ---------------------------------------------------------------

TEST(FNameTest, ConstructFromCString)
{
	FName name("TestName");
	EXPECT_FALSE(name.IsNone());
	EXPECT_GT(name.GetIndex(), 0u);
}

TEST(FNameTest, ConstructFromStdString)
{
	std::string str = "StdStringName";
	FName name(str);
	EXPECT_FALSE(name.IsNone());
	EXPECT_STREQ(*name, "StdStringName");
}

TEST(FNameTest, ConstructFromStringView)
{
	std::string_view sv = "StringViewName";
	FName name(sv);
	EXPECT_FALSE(name.IsNone());
	EXPECT_STREQ(*name, "StringViewName");
}

TEST(FNameTest, ConstructFromNullptrIsNone)
{
	FName name(static_cast<const char*>(nullptr));
	EXPECT_TRUE(name.IsNone());
	EXPECT_EQ(name, NAME_None);
}

// ---------------------------------------------------------------
// ToString / operator*
// ---------------------------------------------------------------

TEST(FNameTest, ToStringReturnsCorrectValue)
{
	FName name("HelloWorld");
	std::string_view sv = name.ToString();
	EXPECT_EQ(sv, "HelloWorld");
}

TEST(FNameTest, OperatorStarReturnsCorrectCString)
{
	FName name("CStringTest");
	const char* str = *name;
	EXPECT_STREQ(str, "CStringTest");
}

TEST(FNameTest, NAME_NoneToStringIsEmpty)
{
	FName name;
	EXPECT_EQ(name.ToString(), "");
	EXPECT_STREQ(*name, "");
}

// ---------------------------------------------------------------
// Equality / Inequality
// ---------------------------------------------------------------

TEST(FNameTest, SameStringIsEqual)
{
	FName a("Player");
	FName b("Player");
	EXPECT_EQ(a, b);
}

TEST(FNameTest, DifferentStringsAreNotEqual)
{
	FName a("Player");
	FName b("Enemy");
	EXPECT_NE(a, b);
}

TEST(FNameTest, NoneEqualsNone)
{
	FName a;
	FName b;
	EXPECT_EQ(a, b);
}

TEST(FNameTest, NoneNotEqualToNonEmpty)
{
	FName a;
	FName b("Something");
	EXPECT_NE(a, b);
}

// ---------------------------------------------------------------
// Deduplication
// ---------------------------------------------------------------

TEST(FNameTest, SameStringSameIndex)
{
	FName a("Dedup");
	FName b("Dedup");
	EXPECT_EQ(a.GetIndex(), b.GetIndex());
}

TEST(FNameTest, DifferentStringsDifferentIndex)
{
	FName a("Alpha");
	FName b("Beta");
	EXPECT_NE(a.GetIndex(), b.GetIndex());
}

TEST(FNameTest, DeduplicationAcrossConstructionTypes)
{
	FName fromCStr("MixedConstruction");
	std::string stdStr = "MixedConstruction";
	FName fromStdStr(stdStr);
	std::string_view sv = "MixedConstruction";
	FName fromSV(sv);

	EXPECT_EQ(fromCStr.GetIndex(), fromStdStr.GetIndex());
	EXPECT_EQ(fromStdStr.GetIndex(), fromSV.GetIndex());
}

// ---------------------------------------------------------------
// Case Sensitivity
// ---------------------------------------------------------------

TEST(FNameTest, CaseSensitiveDifferentNames)
{
	FName lower("foo");
	FName upper("Foo");
	EXPECT_NE(lower, upper);
	EXPECT_NE(lower.GetIndex(), upper.GetIndex());
}

TEST(FNameTest, CaseSensitiveToString)
{
	FName lower("bar");
	FName upper("Bar");
	EXPECT_EQ(lower.ToString(), "bar");
	EXPECT_EQ(upper.ToString(), "Bar");
}

// ---------------------------------------------------------------
// Empty String Handling
// ---------------------------------------------------------------

TEST(FNameTest, EmptyStringIsNone)
{
	FName fromEmpty("");
	EXPECT_TRUE(fromEmpty.IsNone());
	EXPECT_EQ(fromEmpty, NAME_None);
	EXPECT_EQ(fromEmpty.GetIndex(), 0u);
}

TEST(FNameTest, EmptyStdStringIsNone)
{
	std::string empty;
	FName name(empty);
	EXPECT_TRUE(name.IsNone());
}

TEST(FNameTest, EmptyStringViewIsNone)
{
	std::string_view empty;
	FName name(empty);
	EXPECT_TRUE(name.IsNone());
}

// ---------------------------------------------------------------
// Ordering (operator<)
// ---------------------------------------------------------------

TEST(FNameTest, LessThanComparesIndex)
{
	FName a("OrderFirst");
	FName b("OrderSecond");
	// a was inserted first, so a.GetIndex() < b.GetIndex()
	EXPECT_LT(a.GetIndex(), b.GetIndex());
	EXPECT_TRUE(a < b);
	EXPECT_FALSE(b < a);
}

// ---------------------------------------------------------------
// FNameTable Entry Count
// ---------------------------------------------------------------

TEST(FNameTest, TableGrowsOnNewNames)
{
	FNameTable& table = FNameTable::Get();
	uint32_t before = table.GetEntryCount();

	// Use a unique name unlikely to collide with other tests
	FName unique("TableGrowsOnNewNames_UniqueEntry");
	uint32_t after = table.GetEntryCount();

	EXPECT_EQ(after, before + 1);
}

TEST(FNameTest, TableDoesNotGrowOnDuplicate)
{
	FName first("NoDuplicateGrowth");
	FNameTable& table = FNameTable::Get();
	uint32_t before = table.GetEntryCount();

	FName second("NoDuplicateGrowth");
	uint32_t after = table.GetEntryCount();

	EXPECT_EQ(after, before);
}
