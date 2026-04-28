// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "AutomationTest/AutomationTestRegistry.h"
#include "AutomationTestAPI.generated.h"

#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
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

namespace AutomationTest_Private
{

template <typename TValue>
concept CStreamWritable = requires(std::ostringstream& Stream, const TValue& Value)
{
    Stream << Value;
};

AUTOMATIONTEST_API void AddActiveAutomationFailure(
    std::string Message,
    std::source_location Location = std::source_location::current());

} // namespace AutomationTest_Private

AUTOMATIONTEST_API bool TestTrue(
    std::string_view Message,
    bool bCondition,
    std::source_location Location = std::source_location::current());

template <typename TCondition>
bool TestTrue(
    std::string_view Message,
    const TCondition& Condition,
    std::source_location Location = std::source_location::current())
{
    return TestTrue(Message, static_cast<bool>(Condition), Location);
}

AUTOMATIONTEST_API bool TestFalse(
    std::string_view Message,
    bool bCondition,
    std::source_location Location = std::source_location::current());

template <typename TCondition>
bool TestFalse(
    std::string_view Message,
    const TCondition& Condition,
    std::source_location Location = std::source_location::current())
{
    return TestFalse(Message, static_cast<bool>(Condition), Location);
}

template <typename TActual, typename TExpected>
bool TestEqual(
    std::string_view Message,
    const TActual& Actual,
    const TExpected& Expected,
    std::source_location Location = std::source_location::current())
{
    if (Actual == Expected)
    {
        return true;
    }

    std::ostringstream Stream;
    Stream << Message;
    if constexpr (AutomationTest_Private::CStreamWritable<TActual>
        && AutomationTest_Private::CStreamWritable<TExpected>)
    {
        Stream << " Expected: " << Expected << " Actual: " << Actual;
    }

    AutomationTest_Private::AddActiveAutomationFailure(Stream.str(), Location);
    return false;
}

template <typename TActual, typename TExpected>
bool TestNotEqual(
    std::string_view Message,
    const TActual& Actual,
    const TExpected& Expected,
    std::source_location Location = std::source_location::current())
{
    if (Actual != Expected)
    {
        return true;
    }

    std::ostringstream Stream;
    Stream << Message;
    if constexpr (AutomationTest_Private::CStreamWritable<TActual>
        && AutomationTest_Private::CStreamWritable<TExpected>)
    {
        Stream << " Expected not equal to: " << Expected;
    }

    AutomationTest_Private::AddActiveAutomationFailure(Stream.str(), Location);
    return false;
}

template <typename TActual, typename TExpected, typename TTolerance>
bool TestNear(
    std::string_view Message,
    const TActual& Actual,
    const TExpected& Expected,
    const TTolerance& Tolerance,
    std::source_location Location = std::source_location::current())
{
    const long double ActualValue = static_cast<long double>(Actual);
    const long double ExpectedValue = static_cast<long double>(Expected);
    const long double ToleranceValue = static_cast<long double>(Tolerance);
    const long double Difference = ActualValue >= ExpectedValue
        ? ActualValue - ExpectedValue
        : ExpectedValue - ActualValue;

    if (Difference <= ToleranceValue)
    {
        return true;
    }

    std::ostringstream Stream;
    Stream << Message << " Difference: " << Difference << " Tolerance: " << ToleranceValue;
    AutomationTest_Private::AddActiveAutomationFailure(Stream.str(), Location);
    return false;
}

template <typename TActual, typename TExpected>
bool TestGreaterThan(
    std::string_view Message,
    const TActual& Actual,
    const TExpected& Expected,
    std::source_location Location = std::source_location::current())
{
    if (Actual > Expected)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

template <typename TActual, typename TExpected>
bool TestGreaterThanOrEqual(
    std::string_view Message,
    const TActual& Actual,
    const TExpected& Expected,
    std::source_location Location = std::source_location::current())
{
    if (Actual >= Expected)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

template <typename TActual, typename TExpected>
bool TestLessThan(
    std::string_view Message,
    const TActual& Actual,
    const TExpected& Expected,
    std::source_location Location = std::source_location::current())
{
    if (Actual < Expected)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

template <typename TActual, typename TExpected>
bool TestLessThanOrEqual(
    std::string_view Message,
    const TActual& Actual,
    const TExpected& Expected,
    std::source_location Location = std::source_location::current())
{
    if (Actual <= Expected)
    {
        return true;
    }

    AutomationTest_Private::AddActiveAutomationFailure(std::string(Message), Location);
    return false;
}

AUTOMATIONTEST_API bool TestStringEqual(
    std::string_view Message,
    const char* Actual,
    const char* Expected,
    std::source_location Location = std::source_location::current());

/// Optional base for automation tests that share setup and teardown state.
class AUTOMATIONTEST_API FAutomationTestFixture
{
public:
    virtual ~FAutomationTestFixture() = default;

protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
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

    template <typename TCondition>
    bool TestTrue(
        std::string_view Message,
        const TCondition& Condition,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestTrue(Message, Condition, Location);
    }

    template <typename TActual, typename TExpected>
    bool TestEqual(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestEqual(Message, Actual, Expected, Location);
    }

    bool TestFalse(
        std::string_view Message,
        bool bCondition,
        std::source_location Location = std::source_location::current()) const;

    template <typename TCondition>
    bool TestFalse(
        std::string_view Message,
        const TCondition& Condition,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestFalse(Message, Condition, Location);
    }

    template <typename TActual, typename TExpected>
    bool TestNotEqual(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestNotEqual(Message, Actual, Expected, Location);
    }

    template <typename TActual, typename TExpected, typename TTolerance>
    bool TestNear(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        const TTolerance& Tolerance,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestNear(Message, Actual, Expected, Tolerance, Location);
    }

    template <typename TActual, typename TExpected>
    bool TestGreaterThan(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestGreaterThan(Message, Actual, Expected, Location);
    }

    template <typename TActual, typename TExpected>
    bool TestGreaterThanOrEqual(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestGreaterThanOrEqual(Message, Actual, Expected, Location);
    }

    template <typename TActual, typename TExpected>
    bool TestLessThan(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestLessThan(Message, Actual, Expected, Location);
    }

    template <typename TActual, typename TExpected>
    bool TestLessThanOrEqual(
        std::string_view Message,
        const TActual& Actual,
        const TExpected& Expected,
        std::source_location Location = std::source_location::current()) const
    {
        return Enigma::TestLessThanOrEqual(Message, Actual, Expected, Location);
    }

    bool TestStringEqual(
        std::string_view Message,
        const char* Actual,
        const char* Expected,
        std::source_location Location = std::source_location::current()) const;

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
