// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Models;

/// <summary>
/// Entry point for Enigma automation test workflows.
/// Validates CLI options and drives automation-test build target generation.
/// </summary>
public sealed class AutomationTestCommand : ICommand
{
    public string Name => "automation-test";

    public string Description => "List, run, or generate IDE files for automation tests.";

    public BuildResult Execute(BuildOptions options)
    {
        if (!AutomationTestOptions.TryCreate(options, out var automationOptions, out var error))
        {
            return BuildResult.Fail("Invalid automation-test options.", error);
        }

        Console.WriteLine("[AutomationTest] Options parsed.");
        Console.WriteLine($"  Root:       {automationOptions!.RootPath}");
        Console.WriteLine($"  Mode:       {(automationOptions.EngineMode ? "Engine" : "Project")}");
        Console.WriteLine($"  Action:     {automationOptions.ActionName}");
        Console.WriteLine($"  Profile:    {AutomationTestProfiles.ToCliName(automationOptions.Profile)}");

        if (!string.IsNullOrWhiteSpace(automationOptions.Name))
            Console.WriteLine($"  Name:       {automationOptions.Name}");
        if (!string.IsNullOrWhiteSpace(automationOptions.NamePrefix))
            Console.WriteLine($"  Prefix:     {automationOptions.NamePrefix}");
        if (!string.IsNullOrWhiteSpace(automationOptions.Module))
            Console.WriteLine($"  Module:     {automationOptions.Module}");
        if (automationOptions.Tags.Count > 0)
            Console.WriteLine($"  Tags:       {string.Join(", ", automationOptions.Tags)}");
        if (!string.IsNullOrWhiteSpace(automationOptions.ReportDirectory))
            Console.WriteLine($"  Report:     {automationOptions.ReportDirectory}");
        if (automationOptions.AllowEmpty)
            Console.WriteLine("  AllowEmpty: true");

        try
        {
            var scanResult = automationOptions.EngineMode
                ? AutomationTestScanner.ScanEngine(automationOptions.RootPath)
                : AutomationTestScanner.ScanProject(automationOptions.RootPath);

            Console.WriteLine($"  TestSources:{scanResult.Sources.Count}");

            if (automationOptions.GenerateIde)
            {
                return new AutomationTestInvoker().GenerateBuildFiles(
                    scanResult,
                    automationOptions,
                    out _);
            }

            return new AutomationTestInvoker().Invoke(scanResult, automationOptions);
        }
        catch (FileNotFoundException ex)
        {
            return BuildResult.Fail("Automation test scan failed.", ex.Message);
        }
        catch (DirectoryNotFoundException ex)
        {
            return BuildResult.Fail("Automation test scan failed.", ex.Message);
        }
        catch (InvalidOperationException ex)
        {
            return BuildResult.Fail("Automation test scan failed.", ex.Message);
        }

    }
}
