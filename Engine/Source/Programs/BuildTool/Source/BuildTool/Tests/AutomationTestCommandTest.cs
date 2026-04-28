// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// Tests automation-test command option parsing.
/// </summary>
public static class AutomationTestCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== AutomationTestCommand Tests ===");
        Console.WriteLine();

        TestParsesEngineListOptions();
        TestParsesRunFiltersAndReport();
        TestRejectsMissingAction();
        TestRejectsMultipleActions();
        TestRejectsInvalidProfile();
        TestRejectsEmptyValuedOption();
        TestRejectsFlagValue();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestParsesEngineListOptions()
    {
        Console.WriteLine("[Test 1] Parse engine list options");

        var options = Parse(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["engine"] = "",
            ["list"] = "",
            ["profile"] = "all-non-perf",
            ["module"] = "Core",
            ["tag"] = "Unit,Smoke;Core",
            ["allow-empty"] = "",
        });

        Assert(options.EngineMode, "Engine mode should be enabled");
        Assert(options.List, "List action should be enabled");
        Assert(!options.Run, "Run action should not be enabled");
        Assert(!options.GenerateIde, "GenerateIde action should not be enabled");
        Assert(options.Profile == AutomationTestProfile.AllNonPerf, "Profile should be all-non-perf");
        Assert(options.Module == "Core", "Module filter should parse");
        Assert(options.Tags.SequenceEqual(["Unit", "Smoke", "Core"]), "Tags should split comma and semicolon values");
        Assert(options.AllowEmpty, "AllowEmpty should be enabled");

        Console.WriteLine("  PASSED");
    }

    private static void TestParsesRunFiltersAndReport()
    {
        Console.WriteLine("[Test 2] Parse run filters and report path");

        var options = Parse(new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["run"] = "",
            ["profile"] = "ci-standard",
            ["name"] = "System.Core.Name.DefaultConstructionIsNone",
            ["name-prefix"] = "System.Core",
            ["report"] = "Saved/AutomationReports",
        }, "F:/Project/TestGame.eproject");

        Assert(!options.EngineMode, "Project mode should be the default");
        Assert(options.Run, "Run action should be enabled");
        Assert(options.Profile == AutomationTestProfile.CiStandard, "Profile should be ci-standard");
        Assert(options.Name == "System.Core.Name.DefaultConstructionIsNone", "Exact name should parse");
        Assert(options.NamePrefix == "System.Core", "Name prefix should parse");
        Assert(options.ReportDirectory == "Saved/AutomationReports", "Report path should parse");

        Console.WriteLine("  PASSED");
    }

    private static void TestRejectsMissingAction()
    {
        Console.WriteLine("[Test 3] Reject missing action");

        AssertInvalid(
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["engine"] = "",
            },
            "One action is required");

        Console.WriteLine("  PASSED");
    }

    private static void TestRejectsMultipleActions()
    {
        Console.WriteLine("[Test 4] Reject multiple actions");

        AssertInvalid(
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["list"] = "",
                ["run"] = "",
            },
            "Only one action");

        Console.WriteLine("  PASSED");
    }

    private static void TestRejectsInvalidProfile()
    {
        Console.WriteLine("[Test 5] Reject invalid profile");

        AssertInvalid(
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["list"] = "",
                ["profile"] = "nightly",
            },
            "Invalid --profile");

        Console.WriteLine("  PASSED");
    }

    private static void TestRejectsEmptyValuedOption()
    {
        Console.WriteLine("[Test 6] Reject empty valued option");

        AssertInvalid(
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["run"] = "",
                ["name"] = "",
            },
            "--name requires a value");

        Console.WriteLine("  PASSED");
    }

    private static void TestRejectsFlagValue()
    {
        Console.WriteLine("[Test 7] Reject invalid flag value");

        AssertInvalid(
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["engine"] = "maybe",
                ["list"] = "",
            },
            "--engine is a flag");

        Console.WriteLine("  PASSED");
    }

    private static AutomationTestOptions Parse(
        IReadOnlyDictionary<string, string> extraArguments,
        string projectPath = "F:/Project")
    {
        var buildOptions = new BuildOptions
        {
            ProjectPath = projectPath,
            ExtraArguments = extraArguments,
        };

        bool success = AutomationTestOptions.TryCreate(buildOptions, out var options, out var error);
        Assert(success, $"Expected valid options, got error: {error}");
        Assert(options is not null, "Parsed options should not be null");
        return options!;
    }

    private static void AssertInvalid(
        IReadOnlyDictionary<string, string> extraArguments,
        string expectedErrorFragment)
    {
        var buildOptions = new BuildOptions
        {
            ProjectPath = "F:/Project",
            ExtraArguments = extraArguments,
        };

        bool success = AutomationTestOptions.TryCreate(buildOptions, out var options, out var error);
        Assert(!success, "Options should be invalid");
        Assert(options is null, "Invalid parse should not return options");
        Assert(
            error?.Contains(expectedErrorFragment, StringComparison.OrdinalIgnoreCase) == true,
            $"Expected error containing '{expectedErrorFragment}', got '{error}'");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception($"Assertion failed: {message}");
        }
    }
}
