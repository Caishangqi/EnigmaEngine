// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTestContext.h"

#include <utility>

namespace Enigma
{

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

FAutomationTestBase::FAutomationTestBase(std::string InTestName)
    : TestName(std::move(InTestName))
{
}

void FAutomationTestBase::SetContext(const FAutomationTestContext* InContext) noexcept
{
    CurrentContext = InContext;
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

    AddFailure(std::string(Message), Location);
    return false;
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
    }
}

} // namespace Enigma
