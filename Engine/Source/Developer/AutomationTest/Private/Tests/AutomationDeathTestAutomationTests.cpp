// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTest.h"

#include "Misc/AssertionMacros.h"

namespace Enigma
{

ENIGMA_IMPLEMENT_AUTOMATION_TEST(
    FAutomationExpectFatalAssertionTest,
    "System.AutomationTest.DeathTest.ExpectFatalAssert",
    AutomationTest,
    EAutomationTestType::Unit,
    EAutomationTestFlags::None)
{
    ENIGMA_EXPECT_FATAL_ASSERT(
        checkf(false, "Expected fatal assertion value {}", 42),
        "Expected fatal assertion value 42");
}

ENIGMA_IMPLEMENT_AUTOMATION_TEST(
    FAutomationAssertFatalAssertionTest,
    "System.AutomationTest.DeathTest.AssertFatalAssert",
    AutomationTest,
    EAutomationTestType::Unit,
    EAutomationTestFlags::None)
{
    ENIGMA_ASSERT_FATAL_ASSERT(
        checkf(false, "Expected asserted fatal value {}", 7),
        "Expected asserted fatal value 7");
}

} // namespace Enigma
