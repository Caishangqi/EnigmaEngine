// Copyright EnigmaEngine. All Rights Reserved.

#include "AutomationTestReportWriter.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace Enigma
{

namespace
{

using FJson = nlohmann::json;
namespace fs = std::filesystem;

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

std::string ToString(EAutomationTestCaseStatus Status)
{
    switch (Status)
    {
    case EAutomationTestCaseStatus::NotRun:
        return "NotRun";
    case EAutomationTestCaseStatus::Passed:
        return "Passed";
    case EAutomationTestCaseStatus::Failed:
        return "Failed";
    case EAutomationTestCaseStatus::Skipped:
        return "Skipped";
    }

    return "Unknown";
}

std::vector<std::string> GetFlagNames(EAutomationTestFlags Flags)
{
    std::vector<std::string> Names;

    if (EnumHasAnyFlags(Flags, EAutomationTestFlags::Slow))
    {
        Names.emplace_back("Slow");
    }
    if (EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresEngine))
    {
        Names.emplace_back("RequiresEngine");
    }
    if (EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresApplicationCore))
    {
        Names.emplace_back("RequiresApplicationCore");
    }
    if (EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresWindow))
    {
        Names.emplace_back("RequiresWindow");
    }
    if (EnumHasAnyFlags(Flags, EAutomationTestFlags::RequiresProject))
    {
        Names.emplace_back("RequiresProject");
    }
    if (EnumHasAnyFlags(Flags, EAutomationTestFlags::Disabled))
    {
        Names.emplace_back("Disabled");
    }

    return Names;
}

bool HasParentReference(const fs::path& Path)
{
    for (const auto& Part : Path)
    {
        if (Part == "..")
        {
            return true;
        }
    }

    return false;
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

FJson BuildCaseJson(const FAutomationTestCaseReport& Test)
{
    FJson Failures = FJson::array();
    for (const auto& Failure : Test.Failures)
    {
        Failures.push_back({
            { "file", Failure.File },
            { "line", Failure.Line },
            { "message", Failure.Message },
        });
    }

    return {
        { "name", Test.Name },
        { "module", Test.ModuleName },
        { "type", ToString(Test.Type) },
        { "flags", GetFlagNames(Test.Flags) },
        { "tags", Test.Tags },
        { "source", {
            { "file", Test.SourceFile },
            { "line", Test.SourceLine },
        } },
        { "status", ToString(Test.Status) },
        { "durationMs", Test.DurationMs },
        { "failures", Failures },
    };
}

} // namespace

bool FAutomationTestReportWriter::ResolveJsonReportPath(
    std::string_view ReportPath,
    std::filesystem::path& OutPath,
    std::string& OutError)
{
    if (ReportPath.empty())
    {
        OutError = "Report path is empty.";
        return false;
    }

    fs::path RawPath{ std::string(ReportPath) };
    if (RawPath.empty())
    {
        OutError = "Report path is empty.";
        return false;
    }

    if (HasParentReference(RawPath))
    {
        OutError = "Report path must not contain '..' segments.";
        return false;
    }

    fs::path NormalizedPath = RawPath.lexically_normal();
    if (!NormalizedPath.has_extension())
    {
        NormalizedPath /= "AutomationTestReport.json";
    }

    if (NormalizedPath.extension() != ".json")
    {
        OutError = "Automation report path must use a .json file extension.";
        return false;
    }

    OutPath = NormalizedPath;
    return true;
}

bool FAutomationTestReportWriter::WriteJsonReport(
    const FAutomationTestRunReport& Report,
    const std::filesystem::path& ReportPath,
    std::string& OutError)
{
    try
    {
        const fs::path ParentPath = ReportPath.parent_path();
        if (!ParentPath.empty())
        {
            fs::create_directories(ParentPath);
        }

        FJson Tests = FJson::array();
        for (const auto& Test : Report.Tests)
        {
            Tests.push_back(BuildCaseJson(Test));
        }

        const FJson Root = {
            { "schemaVersion", 1 },
            { "profile", Report.Profile },
            { "filter", Report.FilterDescription },
            { "summary", {
                { "total", static_cast<int>(Report.Tests.size()) },
                { "passed", CountStatus(Report, EAutomationTestCaseStatus::Passed) },
                { "failed", CountStatus(Report, EAutomationTestCaseStatus::Failed) },
                { "skipped", CountStatus(Report, EAutomationTestCaseStatus::Skipped) },
                { "notRun", CountStatus(Report, EAutomationTestCaseStatus::NotRun) },
                { "durationMs", Report.DurationMs },
                { "exitCode", Report.ExitCode },
            } },
            { "tests", Tests },
        };

        std::ofstream Output(ReportPath, std::ios::binary);
        if (!Output)
        {
            OutError = "Failed to open automation report for writing.";
            return false;
        }

        Output << Root.dump(2) << '\n';
        return true;
    }
    catch (const std::exception& Ex)
    {
        OutError = Ex.what();
        return false;
    }
}

} // namespace Enigma
