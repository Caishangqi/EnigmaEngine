// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTestReportWriter.h"

#include "AutomationTest/AutomationTest.h"

#if ENIGMA_WITH_AUTOMATION_TESTS
    #include <gtest/gtest.h>
#endif

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Enigma
{

namespace
{

enum class ERunnerAction : uint8_t
{
    List,
    Run
};

enum class ERunnerProfile : uint8_t
{
    LocalFast,
    CiStandard,
    AllNonPerf,
    Perf
};

enum class EValueParseResult : uint8_t
{
    NoMatch,
    Matched,
    Error
};

struct FRunnerOptions
{
    ERunnerAction Action = ERunnerAction::Run;
    ERunnerProfile Profile = ERunnerProfile::LocalFast;
    std::string ProfileName = "local-fast";
    std::string Name;
    std::string NamePrefix;
    std::string ModuleName;
    std::vector<std::string> Tags;
    std::string ReportPath;
    bool bAllowEmpty = false;
    bool bIncludeDisabled = false;
    bool bGoogleDeathTestChildProcess = false;
    std::vector<std::string> GoogleTestArguments;
};

constexpr int GExitSuccess = 0;
constexpr int GExitTestFailure = 1;
constexpr int GExitInvalidArguments = 2;
constexpr int GExitNoTests = 3;
constexpr int GExitReportFailure = 4;

bool StartsWith(std::string_view Value, std::string_view Prefix) noexcept
{
    return Value.size() >= Prefix.size() && Value.substr(0, Prefix.size()) == Prefix;
}

std::string ToLower(std::string_view Value)
{
    std::string Result(Value);
    std::transform(Result.begin(), Result.end(), Result.begin(), [](unsigned char Character)
    {
        return static_cast<char>(std::tolower(Character));
    });
    return Result;
}

bool EqualsIgnoreCase(std::string_view Left, std::string_view Right)
{
    return ToLower(Left) == ToLower(Right);
}

std::string ToString(EAutomationTestType Type)
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

    return "Unknown";
}

std::string ToString(ERunnerProfile Profile)
{
    switch (Profile)
    {
    case ERunnerProfile::LocalFast:
        return "local-fast";
    case ERunnerProfile::CiStandard:
        return "ci-standard";
    case ERunnerProfile::AllNonPerf:
        return "all-non-perf";
    case ERunnerProfile::Perf:
        return "perf";
    }

    return "local-fast";
}

bool TryParseProfile(std::string_view Value, ERunnerProfile& OutProfile)
{
    const std::string Normalized = ToLower(Value);
    if (Normalized == "local-fast")
    {
        OutProfile = ERunnerProfile::LocalFast;
        return true;
    }
    if (Normalized == "ci-standard")
    {
        OutProfile = ERunnerProfile::CiStandard;
        return true;
    }
    if (Normalized == "all-non-perf")
    {
        OutProfile = ERunnerProfile::AllNonPerf;
        return true;
    }
    if (Normalized == "perf")
    {
        OutProfile = ERunnerProfile::Perf;
        return true;
    }

    return false;
}

bool IsOptionToken(std::string_view Value)
{
    return StartsWith(Value, "--");
}

bool IsGoogleDeathTestChildProcess(int Argc, char** Argv)
{
    for (int Index = 1; Index < Argc; ++Index)
    {
        if (StartsWith(Argv[Index], "--gtest_internal_run_death_test"))
        {
            return true;
        }
    }

    return false;
}

EValueParseResult ConsumeValueOption(
    int& Index,
    int Argc,
    char** Argv,
    std::string_view Argument,
    std::string_view OptionName,
    std::string& OutValue,
    std::string& OutError)
{
    const std::string Prefix = std::string(OptionName) + "=";
    if (Argument == OptionName)
    {
        if (Index + 1 >= Argc || IsOptionToken(Argv[Index + 1]))
        {
            OutError = std::string(OptionName) + " requires a value.";
            return EValueParseResult::Error;
        }

        OutValue = Argv[++Index];
        return EValueParseResult::Matched;
    }

    if (StartsWith(Argument, Prefix))
    {
        OutValue = std::string(Argument.substr(Prefix.size()));
        if (OutValue.empty())
        {
            OutError = std::string(OptionName) + " requires a value.";
            return EValueParseResult::Error;
        }

        return EValueParseResult::Matched;
    }

    return EValueParseResult::NoMatch;
}

bool TryConsumeValueOption(
    int& Index,
    int Argc,
    char** Argv,
    std::string_view Argument,
    std::string_view OptionName,
    std::string& OutValue,
    std::string& OutError,
    bool& bMatched)
{
    const EValueParseResult Result = ConsumeValueOption(
        Index,
        Argc,
        Argv,
        Argument,
        OptionName,
        OutValue,
        OutError);
    bMatched = Result != EValueParseResult::NoMatch;
    return Result != EValueParseResult::Error;
}

void PrintUsage()
{
    std::cout
        << "AutomationTestRunner\n"
        << "Usage:\n"
        << "  AutomationTestRunner --list [filters]\n"
        << "  AutomationTestRunner --run [filters] [--report <path>]\n\n"
        << "Filters:\n"
        << "  --profile <local-fast|ci-standard|all-non-perf|perf>\n"
        << "  --name <Exact.Enigma.Name>\n"
        << "  --name-prefix <Enigma.Name.Prefix>\n"
        << "  --module <ModuleName>\n"
        << "  --tag <TagName>                 May be repeated\n"
        << "  --include-disabled\n"
        << "  --allow-empty\n\n"
        << "Reports:\n"
        << "  --report <file-or-directory>    Writes Enigma JSON report\n"
        << "  --gtest-output <xml:path|json:path>\n"
        << "  --gtest_output=<xml:path|json:path>\n";
}

bool ParseArguments(int Argc, char** Argv, FRunnerOptions& OutOptions, std::string& OutError)
{
    bool bActionSpecified = false;
    OutOptions.bGoogleDeathTestChildProcess = IsGoogleDeathTestChildProcess(Argc, Argv);

    for (int Index = 1; Index < Argc; ++Index)
    {
        const std::string_view Argument = Argv[Index];

        if (Argument == "--help" || Argument == "-h")
        {
            PrintUsage();
            std::exit(GExitSuccess);
        }

        if (Argument == "--list" || Argument == "--run")
        {
            if (bActionSpecified)
            {
                OutError = "Only one action may be specified.";
                return false;
            }

            OutOptions.Action = Argument == "--list"
                ? ERunnerAction::List
                : ERunnerAction::Run;
            bActionSpecified = true;
            continue;
        }

        if (Argument == "--allow-empty")
        {
            OutOptions.bAllowEmpty = true;
            continue;
        }

        if (Argument == "--include-disabled")
        {
            OutOptions.bIncludeDisabled = true;
            continue;
        }

        bool bMatched = false;
        std::string Value;
        if (!TryConsumeValueOption(Index, Argc, Argv, Argument, "--profile", Value, OutError, bMatched))
        {
            return false;
        }
        if (bMatched)
        {
            if (!TryParseProfile(Value, OutOptions.Profile))
            {
                OutError = "Invalid --profile value. Valid values: local-fast, ci-standard, all-non-perf, perf.";
                return false;
            }

            OutOptions.ProfileName = ToString(OutOptions.Profile);
            continue;
        }

        if (!TryConsumeValueOption(Index, Argc, Argv, Argument, "--name", OutOptions.Name, OutError, bMatched))
        {
            return false;
        }
        if (bMatched)
        {
            continue;
        }

        if (!TryConsumeValueOption(Index, Argc, Argv, Argument, "--name-prefix", OutOptions.NamePrefix, OutError, bMatched))
        {
            return false;
        }
        if (bMatched)
        {
            continue;
        }

        if (!TryConsumeValueOption(Index, Argc, Argv, Argument, "--module", OutOptions.ModuleName, OutError, bMatched))
        {
            return false;
        }
        if (bMatched)
        {
            continue;
        }

        if (!TryConsumeValueOption(Index, Argc, Argv, Argument, "--tag", Value, OutError, bMatched))
        {
            return false;
        }
        if (bMatched)
        {
            OutOptions.Tags.push_back(Value);
            continue;
        }

        if (!TryConsumeValueOption(Index, Argc, Argv, Argument, "--report", OutOptions.ReportPath, OutError, bMatched))
        {
            return false;
        }
        if (bMatched)
        {
            continue;
        }

        if (!TryConsumeValueOption(Index, Argc, Argv, Argument, "--gtest-output", Value, OutError, bMatched))
        {
            return false;
        }
        if (bMatched)
        {
            OutOptions.GoogleTestArguments.push_back("--gtest_output=" + Value);
            continue;
        }

        if (StartsWith(Argument, "--gtest_filter"))
        {
            if (OutOptions.bGoogleDeathTestChildProcess)
            {
                OutOptions.GoogleTestArguments.emplace_back(Argument);
                continue;
            }

            OutError = "Use Enigma filters (--name, --name-prefix, --module, --tag) instead of --gtest_filter.";
            return false;
        }

        if (StartsWith(Argument, "--gtest_"))
        {
            OutOptions.GoogleTestArguments.emplace_back(Argument);
            continue;
        }

        OutError = "Unknown argument: " + std::string(Argument);
        return false;
    }

    return true;
}

bool HasFlagTag(const FAutomationTestDescriptor& Descriptor, std::string_view Tag)
{
    return (EqualsIgnoreCase(Tag, "Slow")
            && EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::Slow))
        || (EqualsIgnoreCase(Tag, "RequiresEngine")
            && EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::RequiresEngine))
        || (EqualsIgnoreCase(Tag, "RequiresApplicationCore")
            && EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::RequiresApplicationCore))
        || (EqualsIgnoreCase(Tag, "RequiresWindow")
            && EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::RequiresWindow))
        || (EqualsIgnoreCase(Tag, "RequiresProject")
            && EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::RequiresProject))
        || (EqualsIgnoreCase(Tag, "Disabled")
            && EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::Disabled));
}

bool HasTag(const FAutomationTestDescriptor& Descriptor, std::string_view Tag)
{
    if (EqualsIgnoreCase(ToString(Descriptor.Type), Tag) || HasFlagTag(Descriptor, Tag))
    {
        return true;
    }

    return std::any_of(Descriptor.Tags.begin(), Descriptor.Tags.end(),
        [Tag](const std::string& Candidate)
        {
            return EqualsIgnoreCase(Candidate, Tag);
        });
}

bool MatchesProfile(const FAutomationTestDescriptor& Descriptor, ERunnerProfile Profile)
{
    const bool bIsPerf = Descriptor.Type == EAutomationTestType::Perf;
    const bool bIsSlow = EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::Slow);
    const bool bRequiresWindow = EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::RequiresWindow);
    const bool bRequiresProject = EnumHasAnyFlags(Descriptor.Flags, EAutomationTestFlags::RequiresProject);

    switch (Profile)
    {
    case ERunnerProfile::LocalFast:
        return (Descriptor.Type == EAutomationTestType::Unit
                || Descriptor.Type == EAutomationTestType::Smoke)
            && !bIsPerf
            && !bIsSlow
            && !bRequiresWindow
            && !bRequiresProject;
    case ERunnerProfile::CiStandard:
        return (Descriptor.Type == EAutomationTestType::Unit
                || Descriptor.Type == EAutomationTestType::Smoke
                || Descriptor.Type == EAutomationTestType::Integration)
            && !bIsPerf
            && !bIsSlow
            && !bRequiresWindow
            && !bRequiresProject;
    case ERunnerProfile::AllNonPerf:
        return !bIsPerf;
    case ERunnerProfile::Perf:
        return bIsPerf;
    }

    return false;
}

bool MatchesOptions(const FAutomationTestDescriptor& Descriptor, const FRunnerOptions& Options)
{
    if (!Options.bIncludeDisabled && Descriptor.IsDisabled())
    {
        return false;
    }

    if (!Options.Name.empty() && Descriptor.Name != Options.Name)
    {
        return false;
    }

    if (!Options.NamePrefix.empty() && !StartsWith(Descriptor.Name, Options.NamePrefix))
    {
        return false;
    }

    if (!Options.ModuleName.empty() && Descriptor.ModuleName != Options.ModuleName)
    {
        return false;
    }

    if (!MatchesProfile(Descriptor, Options.Profile))
    {
        return false;
    }

    return std::all_of(Options.Tags.begin(), Options.Tags.end(),
        [&Descriptor](const std::string& Tag)
        {
            return HasTag(Descriptor, Tag);
        });
}

std::vector<const FAutomationTestDescriptor*> SelectTests(const FRunnerOptions& Options)
{
    std::vector<const FAutomationTestDescriptor*> SelectedTests;
    const auto& Tests = FAutomationTestRegistry::Get().GetTests();

    for (const auto& Descriptor : Tests)
    {
        if (MatchesOptions(Descriptor, Options))
        {
            SelectedTests.push_back(&Descriptor);
        }
    }

    std::sort(SelectedTests.begin(), SelectedTests.end(),
        [](const FAutomationTestDescriptor* Left, const FAutomationTestDescriptor* Right)
        {
            return Left->Name < Right->Name;
        });

    return SelectedTests;
}

std::string BuildFilterDescription(const FRunnerOptions& Options)
{
    std::string Description = "profile=" + Options.ProfileName;

    if (!Options.Name.empty())
    {
        Description += ", name=" + Options.Name;
    }
    if (!Options.NamePrefix.empty())
    {
        Description += ", name-prefix=" + Options.NamePrefix;
    }
    if (!Options.ModuleName.empty())
    {
        Description += ", module=" + Options.ModuleName;
    }
    for (const auto& Tag : Options.Tags)
    {
        Description += ", tag=" + Tag;
    }
    if (Options.bIncludeDisabled)
    {
        Description += ", include-disabled=true";
    }

    return Description;
}

std::string BuildGoogleFilterExpression(
    const std::vector<const FAutomationTestDescriptor*>& SelectedTests)
{
    std::string Expression;
    for (const auto* Test : SelectedTests)
    {
        if (Test == nullptr)
        {
            continue;
        }

        if (!Expression.empty())
        {
            Expression += ':';
        }

        Expression += Test->GoogleTestSuiteName;
        Expression += '.';
        Expression += Test->GoogleTestName;
    }

    return Expression;
}

FAutomationTestCaseReport CreateCaseReport(const FAutomationTestDescriptor& Descriptor)
{
    FAutomationTestCaseReport Report;
    Report.Name = Descriptor.Name;
    Report.ModuleName = Descriptor.ModuleName;
    Report.Type = Descriptor.Type;
    Report.Flags = Descriptor.Flags;
    Report.Tags = Descriptor.Tags;
    Report.SourceFile = Descriptor.SourceFile != nullptr ? Descriptor.SourceFile : "";
    Report.SourceLine = Descriptor.SourceLine;
    return Report;
}

FAutomationTestRunReport CreateRunReport(
    const FRunnerOptions& Options,
    const std::vector<const FAutomationTestDescriptor*>& SelectedTests)
{
    FAutomationTestRunReport Report;
    Report.Profile = Options.ProfileName;
    Report.FilterDescription = BuildFilterDescription(Options);

    for (const auto* Test : SelectedTests)
    {
        if (Test != nullptr)
        {
            Report.Tests.push_back(CreateCaseReport(*Test));
        }
    }

    return Report;
}

int CountStatus(const FAutomationTestRunReport& Report, EAutomationTestCaseStatus Status)
{
    int Count = 0;
    for (const auto& Test : Report.Tests)
    {
        if (Test.Status == Status)
        {
            ++Count;
        }
    }
    return Count;
}

void PrintSelectedTests(const std::vector<const FAutomationTestDescriptor*>& SelectedTests)
{
    for (const auto* Test : SelectedTests)
    {
        if (Test == nullptr)
        {
            continue;
        }

        std::cout
            << '[' << ToString(Test->Type) << "] "
            << Test->Name
            << " (module=" << Test->ModuleName << ") "
            << Test->SourceFile << ':' << Test->SourceLine
            << '\n';
    }

    std::cout << "AutomationTest: listed " << SelectedTests.size() << " test(s)\n";
}

void PrintSummary(const FAutomationTestRunReport& Report)
{
    std::cout
        << "AutomationTest: total=" << Report.Tests.size()
        << " passed=" << CountStatus(Report, EAutomationTestCaseStatus::Passed)
        << " failed=" << CountStatus(Report, EAutomationTestCaseStatus::Failed)
        << " skipped=" << CountStatus(Report, EAutomationTestCaseStatus::Skipped)
        << " not-run=" << CountStatus(Report, EAutomationTestCaseStatus::NotRun)
        << " duration-ms=" << std::fixed << std::setprecision(2) << Report.DurationMs
        << '\n';
}

std::vector<std::string> BuildGoogleTestArguments(
    const char* ProgramName,
    const FRunnerOptions& Options)
{
    std::vector<std::string> Arguments;
    Arguments.emplace_back(ProgramName != nullptr ? ProgramName : "AutomationTestRunner");
    Arguments.insert(
        Arguments.end(),
        Options.GoogleTestArguments.begin(),
        Options.GoogleTestArguments.end());
    return Arguments;
}

#if ENIGMA_WITH_AUTOMATION_TESTS

std::string MakeGoogleTestKey(const FAutomationTestDescriptor& Descriptor)
{
    return Descriptor.GoogleTestSuiteName + "." + Descriptor.GoogleTestName;
}

std::string MakeGoogleTestKey(const testing::TestInfo& TestInfo)
{
    return std::string(TestInfo.test_suite_name()) + "." + TestInfo.name();
}

class FAutomationResultListener final : public testing::EmptyTestEventListener
{
public:
    explicit FAutomationResultListener(
        const std::vector<const FAutomationTestDescriptor*>& SelectedTests,
        FAutomationTestRunReport& InReport)
        : Report(InReport)
    {
        for (size_t Index = 0; Index < SelectedTests.size(); ++Index)
        {
            if (SelectedTests[Index] != nullptr)
            {
                ResultIndexByGoogleName.emplace(MakeGoogleTestKey(*SelectedTests[Index]), Index);
            }
        }
    }

    void OnTestEnd(const testing::TestInfo& TestInfo) override
    {
        auto It = ResultIndexByGoogleName.find(MakeGoogleTestKey(TestInfo));
        if (It == ResultIndexByGoogleName.end() || It->second >= Report.Tests.size())
        {
            return;
        }

        FAutomationTestCaseReport& CaseReport = Report.Tests[It->second];
        const testing::TestResult* Result = TestInfo.result();
        CaseReport.DurationMs = static_cast<double>(Result->elapsed_time());

        if (Result->Skipped())
        {
            CaseReport.Status = EAutomationTestCaseStatus::Skipped;
        }
        else if (Result->Passed())
        {
            CaseReport.Status = EAutomationTestCaseStatus::Passed;
        }
        else
        {
            CaseReport.Status = EAutomationTestCaseStatus::Failed;
        }

        for (int Index = 0; Index < Result->total_part_count(); ++Index)
        {
            const testing::TestPartResult& Part = Result->GetTestPartResult(Index);
            if (!Part.failed())
            {
                continue;
            }

            CaseReport.Failures.push_back({
                .File = Part.file_name() != nullptr ? Part.file_name() : "",
                .Line = Part.line_number(),
                .Message = Part.summary() != nullptr ? Part.summary() : "",
            });
        }
    }

private:
    FAutomationTestRunReport& Report;
    std::unordered_map<std::string, size_t> ResultIndexByGoogleName;
};

#endif

int RunSelectedTests(
    int Argc,
    char** Argv,
    const FRunnerOptions& Options,
    const std::vector<const FAutomationTestDescriptor*>& SelectedTests,
    FAutomationTestRunReport& Report)
{
    (void)Argc;

    std::vector<std::string> GoogleArgumentStrings = BuildGoogleTestArguments(Argv[0], Options);
    std::vector<char*> GoogleArguments;
    GoogleArguments.reserve(GoogleArgumentStrings.size());
    for (auto& Argument : GoogleArgumentStrings)
    {
        GoogleArguments.push_back(Argument.data());
    }

    int GoogleArgc = static_cast<int>(GoogleArguments.size());
    FAutomationGoogleTestBridge::Initialize(&GoogleArgc, GoogleArguments.data());

#if ENIGMA_WITH_AUTOMATION_TESTS
    testing::UnitTest::GetInstance()->listeners().Append(
        new FAutomationResultListener(SelectedTests, Report));
#else
    (void)SelectedTests;
#endif

    if (!Options.bGoogleDeathTestChildProcess)
    {
        const std::string GoogleFilter = BuildGoogleFilterExpression(SelectedTests);
        FAutomationGoogleTestBridge::ApplyGoogleTestFilterExpression(GoogleFilter);
    }

    const auto StartTime = std::chrono::steady_clock::now();
    const int GoogleExitCode = FAutomationGoogleTestBridge::RunAllTests();
    const auto EndTime = std::chrono::steady_clock::now();

    Report.DurationMs = std::chrono::duration<double, std::milli>(EndTime - StartTime).count();
    return GoogleExitCode == 0 ? GExitSuccess : GExitTestFailure;
}

int WriteReportIfRequested(
    const FRunnerOptions& Options,
    const FAutomationTestRunReport& Report)
{
    if (Options.ReportPath.empty())
    {
        return GExitSuccess;
    }

    std::filesystem::path ReportPath;
    std::string Error;
    if (!FAutomationTestReportWriter::ResolveJsonReportPath(
            Options.ReportPath,
            ReportPath,
            Error))
    {
        std::cerr << "AutomationTest: invalid report path: " << Error << '\n';
        return GExitReportFailure;
    }

    if (!FAutomationTestReportWriter::WriteJsonReport(Report, ReportPath, Error))
    {
        std::cerr << "AutomationTest: failed to write report: " << Error << '\n';
        return GExitReportFailure;
    }

    std::cout << "AutomationTest: report=" << ReportPath.string() << '\n';
    return GExitSuccess;
}

} // namespace

} // namespace Enigma

int main(int Argc, char** Argv)
{
    Enigma::FRunnerOptions Options;
    std::string Error;
    if (!Enigma::ParseArguments(Argc, Argv, Options, Error))
    {
        std::cerr << "AutomationTest: " << Error << '\n';
        Enigma::PrintUsage();
        return Enigma::GExitInvalidArguments;
    }

    const auto SelectedTests = Enigma::SelectTests(Options);

    if (Options.Action == Enigma::ERunnerAction::List)
    {
        Enigma::PrintSelectedTests(SelectedTests);

        auto Report = Enigma::CreateRunReport(Options, SelectedTests);
        Report.ExitCode = Enigma::GExitSuccess;
        const int ReportExitCode = Enigma::WriteReportIfRequested(Options, Report);
        return ReportExitCode == Enigma::GExitSuccess ? Enigma::GExitSuccess : ReportExitCode;
    }

    auto Report = Enigma::CreateRunReport(Options, SelectedTests);

    if (SelectedTests.empty())
    {
        std::cerr << "AutomationTest: no tests matched " << Report.FilterDescription << '\n';
        Report.ExitCode = Options.bAllowEmpty
            ? Enigma::GExitSuccess
            : Enigma::GExitNoTests;
        Enigma::PrintSummary(Report);

        const int ReportExitCode = Enigma::WriteReportIfRequested(Options, Report);
        if (ReportExitCode != Enigma::GExitSuccess)
        {
            return ReportExitCode;
        }

        return Report.ExitCode;
    }

    Report.ExitCode = Enigma::RunSelectedTests(Argc, Argv, Options, SelectedTests, Report);
    Enigma::PrintSummary(Report);

    const int ReportExitCode = Enigma::WriteReportIfRequested(Options, Report);
    if (ReportExitCode != Enigma::GExitSuccess)
    {
        return ReportExitCode;
    }

    return Report.ExitCode;
}
