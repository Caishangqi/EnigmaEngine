// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTest/AutomationTestRegistry.h"

#include "Logging/LogMacros.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleMacros.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace Enigma
{

DEFINE_LOG_CATEGORY_STATIC(LogAutomationTest, Info, All);

namespace
{

constexpr uint32_t GKnownAutomationTestFlags =
    static_cast<uint32_t>(EAutomationTestFlags::Slow)
    | static_cast<uint32_t>(EAutomationTestFlags::RequiresEngine)
    | static_cast<uint32_t>(EAutomationTestFlags::RequiresApplicationCore)
    | static_cast<uint32_t>(EAutomationTestFlags::RequiresWindow)
    | static_cast<uint32_t>(EAutomationTestFlags::RequiresProject)
    | static_cast<uint32_t>(EAutomationTestFlags::Disabled);

bool SetError(std::string* OutError, std::string Error)
{
    if (OutError)
    {
        *OutError = std::move(Error);
    }
    return false;
}

bool IsValidAutomationTestType(EAutomationTestType Type) noexcept
{
    switch (Type)
    {
    case EAutomationTestType::Unit:
    case EAutomationTestType::Integration:
    case EAutomationTestType::Smoke:
    case EAutomationTestType::Perf:
    case EAutomationTestType::Functional:
        return true;
    }

    return false;
}

bool IsValidAutomationTestFlags(EAutomationTestFlags Flags) noexcept
{
    const uint32_t RawFlags = static_cast<uint32_t>(Flags);
    return (RawFlags & ~GKnownAutomationTestFlags) == 0;
}

bool ContainsWhitespace(std::string_view Value) noexcept
{
    return std::any_of(Value.begin(), Value.end(), [](char Character)
    {
        return std::isspace(static_cast<unsigned char>(Character)) != 0;
    });
}

bool HasEmptyNameSegment(std::string_view Name) noexcept
{
    return Name.empty()
        || Name.front() == '.'
        || Name.back() == '.'
        || Name.find("..") != std::string_view::npos;
}

bool EqualsIgnoreCase(std::string_view Left, std::string_view Right) noexcept
{
    if (Left.size() != Right.size())
    {
        return false;
    }

    for (size_t Index = 0; Index < Left.size(); ++Index)
    {
        const auto LeftChar = static_cast<unsigned char>(Left[Index]);
        const auto RightChar = static_cast<unsigned char>(Right[Index]);
        if (std::tolower(LeftChar) != std::tolower(RightChar))
        {
            return false;
        }
    }

    return true;
}

bool StartsWith(std::string_view Value, std::string_view Prefix) noexcept
{
    return Value.size() >= Prefix.size() && Value.substr(0, Prefix.size()) == Prefix;
}

std::string_view GetTypeTag(EAutomationTestType Type) noexcept
{
    switch (Type)
    {
    case EAutomationTestType::Unit:
        return "Unit";
    case EAutomationTestType::Integration:
        return "Integration";
    case EAutomationTestType::Smoke:
        return "Smoke";
    case EAutomationTestType::Perf:
        return "Perf";
    case EAutomationTestType::Functional:
        return "Functional";
    }

    return {};
}

bool HasFlagTag(const FAutomationTestDescriptor& Descriptor, std::string_view Tag) noexcept
{
    const auto Flags = Descriptor.Flags;

    return (EqualsIgnoreCase(Tag, "Slow")
            && EnumHasAnyFlags(Flags, EAutomationTestFlags::Slow))
        || (EqualsIgnoreCase(Tag, "RequiresEngine")
            && EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresEngine))
        || (EqualsIgnoreCase(Tag, "RequiresApplicationCore")
            && EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresApplicationCore))
        || (EqualsIgnoreCase(Tag, "RequiresWindow")
            && EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresWindow))
        || (EqualsIgnoreCase(Tag, "RequiresProject")
            && EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresProject))
        || (EqualsIgnoreCase(Tag, "Disabled")
            && EnumHasAnyFlags(Flags, EAutomationTestFlags::Disabled));
}

bool HasTag(const FAutomationTestDescriptor& Descriptor, std::string_view Tag) noexcept
{
    if (EqualsIgnoreCase(GetTypeTag(Descriptor.Type), Tag) || HasFlagTag(Descriptor, Tag))
    {
        return true;
    }

    return std::any_of(Descriptor.Tags.begin(), Descriptor.Tags.end(),
        [Tag](const std::string& Candidate)
        {
            return EqualsIgnoreCase(Candidate, Tag);
        });
}

} // namespace

FAutomationTestRegistry& FAutomationTestRegistry::Get()
{
    static FAutomationTestRegistry Registry;
    return Registry;
}

bool FAutomationTestRegistry::RegisterTest(
    const FAutomationTestDescriptor& Descriptor,
    std::string* OutError)
{
    if (!ValidateDescriptor(Descriptor, OutError))
    {
        return false;
    }

    Tests.push_back(Descriptor);
    return true;
}

const std::vector<FAutomationTestDescriptor>& FAutomationTestRegistry::GetTests() const noexcept
{
    return Tests;
}

const FAutomationTestDescriptor* FAutomationTestRegistry::FindByName(
    std::string_view Name) const noexcept
{
    auto It = std::find_if(Tests.begin(), Tests.end(),
        [Name](const FAutomationTestDescriptor& Descriptor)
        {
            return Descriptor.Name == Name;
        });

    return It != Tests.end() ? &(*It) : nullptr;
}

std::vector<const FAutomationTestDescriptor*> FAutomationTestRegistry::FilterTests(
    const FAutomationTestFilter& Filter) const
{
    std::vector<const FAutomationTestDescriptor*> Results;

    for (const auto& Descriptor : Tests)
    {
        if (!Filter.bIncludeDisabled && Descriptor.IsDisabled())
        {
            continue;
        }

        if (!Filter.Name.empty() && Descriptor.Name != Filter.Name)
        {
            continue;
        }

        if (!Filter.NamePrefix.empty() && !StartsWith(Descriptor.Name, Filter.NamePrefix))
        {
            continue;
        }

        if (!Filter.ModuleName.empty() && Descriptor.ModuleName != Filter.ModuleName)
        {
            continue;
        }

        const bool bHasAllTags = std::all_of(Filter.Tags.begin(), Filter.Tags.end(),
            [&Descriptor](const std::string& Tag)
            {
                return HasTag(Descriptor, Tag);
            });

        if (!bHasAllTags)
        {
            continue;
        }

        Results.push_back(&Descriptor);
    }

    return Results;
}

void FAutomationTestRegistry::Clear()
{
    Tests.clear();
}

bool FAutomationTestRegistry::ValidateDescriptor(
    const FAutomationTestDescriptor& Descriptor,
    std::string* OutError) const
{
    if (Descriptor.Name.empty())
    {
        return SetError(OutError, "Automation test name is empty.");
    }

    if (HasEmptyNameSegment(Descriptor.Name) || ContainsWhitespace(Descriptor.Name))
    {
        return SetError(OutError, "Automation test name has an invalid hierarchical format.");
    }

    if (Descriptor.ModuleName.empty())
    {
        return SetError(OutError, "Automation test module name is empty.");
    }

    if (!IsValidAutomationTestType(Descriptor.Type))
    {
        return SetError(OutError, "Automation test type is invalid.");
    }

    if (!IsValidAutomationTestFlags(Descriptor.Flags))
    {
        return SetError(OutError, "Automation test flags contain unknown bits.");
    }

    if (Descriptor.SourceFile == nullptr || Descriptor.SourceFile[0] == '\0')
    {
        return SetError(OutError, "Automation test source file is empty.");
    }

    if (Descriptor.SourceLine <= 0)
    {
        return SetError(OutError, "Automation test source line must be positive.");
    }

    if (Descriptor.GoogleTestSuiteName.empty() || Descriptor.GoogleTestName.empty())
    {
        return SetError(OutError, "Automation test GoogleTest names are empty.");
    }

    if (FindByName(Descriptor.Name) != nullptr)
    {
        return SetError(OutError, "Duplicate automation test name: " + Descriptor.Name);
    }

    return true;
}

class FAutomationTestModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
        ENIGMA_LOG(LogAutomationTest, Info, "AutomationTest module started");
    }

    void ShutdownModule() override
    {
        ENIGMA_LOG(LogAutomationTest, Info, "AutomationTest module shut down");
    }

    bool SupportsDynamicReloading() override
    {
        return false;
    }
};

IMPLEMENT_MODULE(FAutomationTestModule, AutomationTest)

} // namespace Enigma
