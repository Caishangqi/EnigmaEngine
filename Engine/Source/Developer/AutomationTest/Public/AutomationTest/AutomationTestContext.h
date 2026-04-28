// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "AutomationTest/AutomationTestRegistry.h"
#include "AutomationTestAPI.generated.h"

#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Enigma
{

/// One assertion failure captured during an automation test.
struct AUTOMATIONTEST_API FAutomationTestFailure
{
    std::string Message;
    std::string File;
    int Line = 0;
};

/// Per-test execution context and failure sink.
class AUTOMATIONTEST_API FAutomationTestContext
{
public:
    FAutomationTestContext(std::string InTestName, std::string InModuleName);

    void AddFailure(
        std::string Message,
        std::source_location Location = std::source_location::current()) const;

    [[nodiscard]] const std::string& GetTestName() const noexcept;
    [[nodiscard]] const std::string& GetModuleName() const noexcept;
    [[nodiscard]] const std::vector<FAutomationTestFailure>& GetFailures() const noexcept;
    [[nodiscard]] bool HasAnyFailures() const noexcept;

private:
    std::string TestName;
    std::string ModuleName;
    mutable std::vector<FAutomationTestFailure> Failures;
};

/// Base class used by UE-style automation test macros.
class AUTOMATIONTEST_API FAutomationTestBase
{
public:
    explicit FAutomationTestBase(std::string InTestName);
    virtual ~FAutomationTestBase() = default;

    virtual bool RunTest(const FAutomationTestContext& Context) = 0;

    void SetContext(const FAutomationTestContext* InContext) noexcept;

    bool TestTrue(
        std::string_view Message,
        bool bCondition,
        std::source_location Location = std::source_location::current()) const;

    template <typename TActual, typename TExpected>
    bool TestEqual(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        std::source_location Location = std::source_location::current()) const
    {
        if (Actual == Expected)
        {
            return true;
        }

        std::ostringstream Stream;
        Stream << Message << " Expected: " << Expected << " Actual: " << Actual;
        AddFailure(Stream.str(), Location);
        return false;
    }

    [[nodiscard]] const std::string& GetTestName() const noexcept;

protected:
    void AddFailure(std::string Message, std::source_location Location) const;

private:
    std::string TestName;
    const FAutomationTestContext* CurrentContext = nullptr;
};

/// Thin wrapper around GoogleTest runner functions used by AutomationTestRunner.
class AUTOMATIONTEST_API FAutomationGoogleTestBridge
{
public:
    static void Initialize(int* Argc, char** Argv);
    static void ApplyFilter(const FAutomationTestFilter& Filter);
    static void ApplyGoogleTestFilterExpression(std::string_view FilterExpression);
    static int RunAllTests();
    static std::string BuildGoogleTestFilter(const FAutomationTestFilter& Filter);
};

} // namespace Enigma
