// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "AutomationTest/AutomationTestFlags.h"
#include "AutomationTestAPI.generated.h"

#include <string>
#include <string_view>
#include <vector>

namespace Enigma
{

/// Metadata for one registered automation test.
struct AUTOMATIONTEST_API FAutomationTestDescriptor
{
    std::string Name;
    std::string ModuleName;
    EAutomationTestType Type = EAutomationTestType::Unit;
    EAutomationTestFlags Flags = EAutomationTestFlags::None;
    std::vector<std::string> Tags;
    const char* SourceFile = nullptr;
    int SourceLine = 0;
    std::string GoogleTestSuiteName;
    std::string GoogleTestName;

    [[nodiscard]] bool IsDisabled() const noexcept
    {
        return EnumHasAnyFlags(Flags, EAutomationTestFlags::Disabled);
    }
};

/// Filter request used by BuildTool and the runner.
struct AUTOMATIONTEST_API FAutomationTestFilter
{
    std::string Name;
    std::string NamePrefix;
    std::string ModuleName;
    std::vector<std::string> Tags;
    bool bIncludeDisabled = false;
};

/// Process-local registry for statically registered automation tests.
class AUTOMATIONTEST_API FAutomationTestRegistry
{
public:
    static FAutomationTestRegistry& Get();

    bool RegisterTest(
        const FAutomationTestDescriptor& Descriptor,
        std::string* OutError = nullptr);

    [[nodiscard]] const std::vector<FAutomationTestDescriptor>& GetTests() const noexcept;

    [[nodiscard]] const FAutomationTestDescriptor* FindByName(std::string_view Name) const noexcept;

    [[nodiscard]] std::vector<const FAutomationTestDescriptor*> FilterTests(
        const FAutomationTestFilter& Filter) const;

    void Clear();

private:
    [[nodiscard]] bool ValidateDescriptor(
        const FAutomationTestDescriptor& Descriptor,
        std::string* OutError) const;

private:
    std::vector<FAutomationTestDescriptor> Tests;
};

} // namespace Enigma
