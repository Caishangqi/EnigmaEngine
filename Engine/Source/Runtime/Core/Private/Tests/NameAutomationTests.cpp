// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Misc/Name.h"

#include <string>
#include <string_view>

namespace Enigma
{

#define ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(TestClass, PrettyName) \
    ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(                          \
        TestClass,                                                     \
        PrettyName,                                                    \
        Core,                                                          \
        EAutomationTestType::Unit,                                     \
        EAutomationTestFlags::None)                                    \
    bool TestClass::RunTest(const FAutomationTestContext& Context)

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameDefaultConstructionIsNoneTest,
    "System.Core.Name.DefaultConstructionIsNone")
{
    const FName Name;
    TestTrue("Default FName should be NAME_None", Name.IsNone());
    TestEqual("NAME_None index should be zero", Name.GetIndex(), 0u);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameDefaultEqualsNameNoneTest,
    "System.Core.Name.DefaultEqualsNAME_None")
{
    const FName Name;
    TestTrue("Default FName should equal NAME_None", Name == NAME_None);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameConstructFromCStringTest,
    "System.Core.Name.ConstructFromCString")
{
    const FName Name("TestName");
    TestTrue("CString name should not be NAME_None", !Name.IsNone());
    TestTrue("CString name should allocate a non-zero index", Name.GetIndex() > 0u);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameConstructFromStdStringTest,
    "System.Core.Name.ConstructFromStdString")
{
    const std::string StringValue = "StdStringName";
    const FName Name(StringValue);
    TestTrue("std::string name should not be NAME_None", !Name.IsNone());
    TestEqual("std::string name should preserve text", std::string(*Name), StringValue);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameConstructFromStringViewTest,
    "System.Core.Name.ConstructFromStringView")
{
    const std::string_view StringViewValue = "StringViewName";
    const FName Name(StringViewValue);
    TestTrue("std::string_view name should not be NAME_None", !Name.IsNone());
    TestEqual("std::string_view name should preserve text", std::string(*Name), std::string(StringViewValue));
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameConstructFromNullptrIsNoneTest,
    "System.Core.Name.ConstructFromNullptrIsNone")
{
    const FName Name(static_cast<const char*>(nullptr));
    TestTrue("Null C string should create NAME_None", Name.IsNone());
    TestTrue("Null C string name should equal NAME_None", Name == NAME_None);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameToStringReturnsCorrectValueTest,
    "System.Core.Name.ToStringReturnsCorrectValue")
{
    const FName Name("HelloWorld");
    TestEqual("ToString should return original text", std::string(Name.ToString()), std::string("HelloWorld"));
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameOperatorStarReturnsCorrectCStringTest,
    "System.Core.Name.OperatorStarReturnsCorrectCString")
{
    const FName Name("CStringTest");
    TestEqual("operator* should return original text", std::string(*Name), std::string("CStringTest"));
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameNoneToStringIsEmptyTest,
    "System.Core.Name.NAME_NoneToStringIsEmpty")
{
    const FName Name;
    TestEqual("NAME_None ToString should be empty", std::string(Name.ToString()), std::string());
    TestEqual("NAME_None operator* should be empty", std::string(*Name), std::string());
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameSameStringIsEqualTest,
    "System.Core.Name.SameStringIsEqual")
{
    const FName Left("Player");
    const FName Right("Player");
    TestTrue("Same strings should produce equal names", Left == Right);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameDifferentStringsAreNotEqualTest,
    "System.Core.Name.DifferentStringsAreNotEqual")
{
    const FName Left("Player");
    const FName Right("Enemy");
    TestTrue("Different strings should produce different names", Left != Right);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameNoneEqualsNoneTest,
    "System.Core.Name.NoneEqualsNone")
{
    const FName Left;
    const FName Right;
    TestTrue("Two NAME_None values should be equal", Left == Right);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameNoneNotEqualToNonEmptyTest,
    "System.Core.Name.NoneNotEqualToNonEmpty")
{
    const FName EmptyName;
    const FName NonEmptyName("Something");
    TestTrue("NAME_None should not equal a non-empty name", EmptyName != NonEmptyName);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameSameStringSameIndexTest,
    "System.Core.Name.SameStringSameIndex")
{
    const FName Left("Dedup");
    const FName Right("Dedup");
    TestEqual("Same strings should share the same index", Left.GetIndex(), Right.GetIndex());
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameDifferentStringsDifferentIndexTest,
    "System.Core.Name.DifferentStringsDifferentIndex")
{
    const FName Left("Alpha");
    const FName Right("Beta");
    TestTrue("Different strings should use different indices", Left.GetIndex() != Right.GetIndex());
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameDeduplicationAcrossConstructionTypesTest,
    "System.Core.Name.DeduplicationAcrossConstructionTypes")
{
    const FName FromCString("MixedConstruction");
    const std::string StringValue = "MixedConstruction";
    const FName FromStdString(StringValue);
    const std::string_view StringViewValue = "MixedConstruction";
    const FName FromStringView(StringViewValue);

    TestEqual("C string and std::string names should share an index",
        FromCString.GetIndex(),
        FromStdString.GetIndex());
    TestEqual("std::string and std::string_view names should share an index",
        FromStdString.GetIndex(),
        FromStringView.GetIndex());
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameCaseSensitiveDifferentNamesTest,
    "System.Core.Name.CaseSensitiveDifferentNames")
{
    const FName Lower("foo");
    const FName Upper("Foo");
    TestTrue("Case-different names should not compare equal", Lower != Upper);
    TestTrue("Case-different names should not share an index", Lower.GetIndex() != Upper.GetIndex());
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameCaseSensitiveToStringTest,
    "System.Core.Name.CaseSensitiveToString")
{
    const FName Lower("bar");
    const FName Upper("Bar");
    TestEqual("Lowercase text should be preserved", std::string(Lower.ToString()), std::string("bar"));
    TestEqual("Uppercase text should be preserved", std::string(Upper.ToString()), std::string("Bar"));
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameEmptyStringIsNoneTest,
    "System.Core.Name.EmptyStringIsNone")
{
    const FName Name("");
    TestTrue("Empty string should create NAME_None", Name.IsNone());
    TestTrue("Empty string name should equal NAME_None", Name == NAME_None);
    TestEqual("Empty string name should use index zero", Name.GetIndex(), 0u);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameEmptyStdStringIsNoneTest,
    "System.Core.Name.EmptyStdStringIsNone")
{
    const std::string EmptyString;
    const FName Name(EmptyString);
    TestTrue("Empty std::string should create NAME_None", Name.IsNone());
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameEmptyStringViewIsNoneTest,
    "System.Core.Name.EmptyStringViewIsNone")
{
    const std::string_view EmptyStringView;
    const FName Name(EmptyStringView);
    TestTrue("Empty std::string_view should create NAME_None", Name.IsNone());
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameLessThanComparesIndexTest,
    "System.Core.Name.LessThanComparesIndex")
{
    const FName First("OrderFirst");
    const FName Second("OrderSecond");
    TestTrue("First inserted name should have a smaller index", First.GetIndex() < Second.GetIndex());
    TestTrue("operator< should compare by index", First < Second);
    TestTrue("operator< should be directional", !(Second < First));
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameTableGrowsOnNewNamesTest,
    "System.Core.Name.TableGrowsOnNewNames")
{
    FNameTable& Table = FNameTable::Get();
    const uint32_t Before = Table.GetEntryCount();
    const std::string UniqueName = "TableGrowsOnNewNames_UniqueEntry_" + std::to_string(Before);

    const FName Unique(UniqueName);
    (void)Unique;

    const uint32_t After = Table.GetEntryCount();
    TestEqual("A new name should add one table entry", After, Before + 1);
    return !Context.HasAnyFailures();
}

ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST(
    FNameTableDoesNotGrowOnDuplicateTest,
    "System.Core.Name.TableDoesNotGrowOnDuplicate")
{
    FNameTable& Table = FNameTable::Get();
    const std::string UniqueName = "NoDuplicateGrowth_" + std::to_string(Table.GetEntryCount());
    const FName First(UniqueName);
    (void)First;

    const uint32_t Before = Table.GetEntryCount();
    const FName Second(UniqueName);
    (void)Second;

    const uint32_t After = Table.GetEntryCount();
    TestEqual("A duplicate name should not add a table entry", After, Before);

    return !Context.HasAnyFailures();
}

#undef ENIGMA_IMPLEMENT_FNAME_AUTOMATION_TEST

} // namespace Enigma
