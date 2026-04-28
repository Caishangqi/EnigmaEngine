// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTestContext.h"

#include <cstring>
#include <utility>

namespace Enigma
{

namespace
{

thread_local const FAutomationTestContext* GActiveAutomationTestContext = nullptr;

} // namespace

FAutomationTestContext::FAutomationTestContext(
    std::string InTestName,
    std::string InModuleName)
    : TestName(std::move(InTestName))
    , ModuleName(std::move(InModuleName))
{
}

void FAutomationTestContext::AddFailure(
    std::string Message,
    std::source_location Location) const
{
    Failures.push_back(FAutomationTestFailure
    {
        .Message = std::move(Message),
        .File = Location.file_name(),
        .Line = static_cast<int>(Location.line()),
    });
}

const std::string& FAutomationTestContext::GetTestName() const noexcept
{
    return TestName;
}

const std::string& FAutomationTestContext::GetModuleName() const noexcept
{
    return ModuleName;
}

const std::vector<FAutomationTestFailure>& FAutomationTestContext::GetFailures() const noexcept
{
    return Failures;
}

bool FAutomationTestContext::HasAnyFailures() const noexcept
{
    return !Failures.empty();
}

namespace AutomationTest_Private
{

void AddActiveAutomationFailure(
    std::string Message,
    std::source_location Location)
{
    if (GActiveAutomationTestContext != nullptr)
    {
        GActiveAutomationTestContext->AddFailure(std::move(Message), Location);
    }
}

} // namespace AutomationTest_Private

bool TestTrue(
    std::string_view Message,
    bool bCondition,
    std::source_location Location)
{
    if (bCondition)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

bool TestFalse(
    std::string_view Message,
    bool bCondition,
    std::source_location Location)
{
    if (!bCondition)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

bool TestStringEqual(
    std::string_view Message,
    const char* Actual,
    const char* Expected,
    std::source_location Location)
{
    if (Actual == nullptr || Expected == nullptr)
    {
        if (Actual == Expected)
        {
            return true;
        }

        AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
        return false;
    }

    if (std::strcmp(Actual, Expected) == 0)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

FAutomationTestBase::FAutomationTestBase(std::string InTestName)
    : TestName(std::move(InTestName))
{
}

void FAutomationTestBase::SetContext(const FAutomationTestContext* InContext) noexcept
{
    CurrentContext = InContext;
    GActiveAutomationTestContext = InContext;
}

bool FAutomationTestBase::TestTrue(
    std::string_view Message,
    bool bCondition,
    std::source_location Location) const
{
    if (bCondition)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

bool FAutomationTestBase::TestFalse(
    std::string_view Message,
    bool bCondition,
    std::source_location Location) const
{
    return Enigma::TestFalse(Message, bCondition, Location);
}

bool FAutomationTestBase::TestStringEqual(
    std::string_view Message,
    const char* Actual,
    const char* Expected,
    std::source_location Location) const
{
    return Enigma::TestStringEqual(Message, Actual, Expected, Location);
}

const std::string& FAutomationTestBase::GetTestName() const noexcept
{
    return TestName;
}

void FAutomationTestBase::AddFailure(
    std::string Message,
    std::source_location Location) const
{
    if (CurrentContext != nullptr)
    {
        CurrentContext->AddFailure(std::move(Message), Location);
        return;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::move(Message), Location);
}

} // namespace Enigma
