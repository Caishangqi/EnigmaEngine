// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"
#include "Misc/Name.h"

#include <string>
#include <string_view>

namespace Enigma
{

ENIGMA_IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNameBasicAutomationTest,
    "System.Core.Name.Basic",
    Core,
    EAutomationTestType::Unit,
    EAutomationTestFlags::None)

bool FNameBasicAutomationTest::RunTest(const FAutomationTestContext& Context)
{
    const FName EmptyName;
    const FName PlayerName("PlayerObject");
    const FName DuplicatePlayerName(std::string_view("PlayerObject"));
    const FName EnemyName(std::string("EnemyObject"));

    TestTrue("Default FName should be NAME_None", EmptyName.IsNone());
    TestEqual("NAME_None index should be zero", EmptyName.GetIndex(), 0u);
    TestEqual("Duplicate names should share the same index",
        PlayerName.GetIndex(),
        DuplicatePlayerName.GetIndex());
    TestTrue("Equal names should compare equal", PlayerName == DuplicatePlayerName);
    TestTrue("Different names should not compare equal", PlayerName != EnemyName);
    TestEqual("FName should resolve to the original string",
        std::string(PlayerName.ToString()),
        std::string("PlayerObject"));

    return !Context.HasAnyFailures();
}

} // namespace Enigma
