// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

#include "AutomationTest/AutomationTestFlags.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Enigma
{

enum class EAutomationTestCaseStatus : uint8_t
{
    NotRun,
    Passed,
    Failed,
    Skipped
};

struct FAutomationTestFailureRecord
{
    std::string File;
    int Line = 0;
    std::string Message;
};

struct FAutomationTestCaseReport
{
    std::string Name;
    std::string ModuleName;
    EAutomationTestType Type = EAutomationTestType::Unit;
    EAutomationTestFlags Flags = EAutomationTestFlags::None;
    std::vector<std::string> Tags;
    std::string SourceFile;
    int SourceLine = 0;
    EAutomationTestCaseStatus Status = EAutomationTestCaseStatus::NotRun;
    double DurationMs = 0.0;
    std::vector<FAutomationTestFailureRecord> Failures;
};

struct FAutomationTestRunReport
{
    std::string Profile;
    std::string FilterDescription;
    int ExitCode = 0;
    double DurationMs = 0.0;
    std::vector<FAutomationTestCaseReport> Tests;
};

class FAutomationTestReportWriter
{
public:
    static bool ResolveJsonReportPath(
        std::string_view ReportPath,
        std::filesystem::path& OutPath,
        std::string& OutError);

    static bool WriteJsonReport(
        const FAutomationTestRunReport& Report,
        const std::filesystem::path& ReportPath,
        std::string& OutError);
};

} // namespace Enigma
