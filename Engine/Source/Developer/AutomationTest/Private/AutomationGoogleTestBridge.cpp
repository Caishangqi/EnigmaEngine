// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTestContext.h"

#if ENIGMA_WITH_AUTOMATION_TESTS
    #include <gtest/gtest.h>
#endif

namespace Enigma
{

void FAutomationGoogleTestBridge::Initialize(int* Argc, char** Argv)
{
#if ENIGMA_WITH_AUTOMATION_TESTS
    ::testing::InitGoogleTest(Argc, Argv);
#else
    (void)Argc;
    (void)Argv;
#endif
}

void FAutomationGoogleTestBridge::ApplyFilter(const FAutomationTestFilter& Filter)
{
#if ENIGMA_WITH_AUTOMATION_TESTS
    const std::string GoogleFilter = BuildGoogleTestFilter(Filter);
    ApplyGoogleTestFilterExpression(GoogleFilter);
#else
    (void)Filter;
#endif
}

void FAutomationGoogleTestBridge::ApplyGoogleTestFilterExpression(
    std::string_view FilterExpression)
{
#if ENIGMA_WITH_AUTOMATION_TESTS
    if (!FilterExpression.empty())
    {
        GTEST_FLAG_SET(filter, std::string(FilterExpression));
    }
#else
    (void)FilterExpression;
#endif
}

int FAutomationGoogleTestBridge::RunAllTests()
{
#if ENIGMA_WITH_AUTOMATION_TESTS
    return RUN_ALL_TESTS();
#else
    return 0;
#endif
}

std::string FAutomationGoogleTestBridge::BuildGoogleTestFilter(
    const FAutomationTestFilter& Filter)
{
    std::string Result;
    const auto Tests = FAutomationTestRegistry::Get().FilterTests(Filter);

    for (const auto* Test : Tests)
    {
        if (Test == nullptr)
        {
            continue;
        }

        if (!Result.empty())
        {
            Result += ':';
        }

        Result += Test->GoogleTestSuiteName;
        Result += '.';
        Result += Test->GoogleTestName;
    }

    return Result;
}

} // namespace Enigma
