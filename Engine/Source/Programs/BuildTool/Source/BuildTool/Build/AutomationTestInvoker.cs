// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using System.Diagnostics;
using System.Text;
using BuildTool.Generators;
using BuildTool.Models;

/// <summary>
/// Generates, builds, and invokes the standalone AutomationTestRunner target.
/// </summary>
public sealed class AutomationTestInvoker
{
    private const string CMakeBuildConfiguration = "Release";
    private const string CMakeGeneratorName = "Visual Studio 17 2022";

    private readonly CMakeInvoker _cmake;

    public AutomationTestInvoker(CMakeInvoker? cmake = null)
    {
        _cmake = cmake ?? new CMakeInvoker();
    }

    public sealed class BuildLayout
    {
        public required string RootPath { get; init; }
        public required string ProjectName { get; init; }
        public required string CMakeSourceDir { get; init; }
        public required string CMakeBuildDir { get; init; }
        public required string CMakeListsPath { get; init; }
        public required string RunnerPath { get; init; }
    }

    /// <summary>
    /// Generate API headers and the disposable CMake project used by automation tests.
    /// </summary>
    public BuildResult GenerateBuildFiles(
        AutomationTestScanner.ScanResult scanResult,
        AutomationTestOptions options,
        out BuildLayout layout)
    {
        layout = CreateLayout(scanResult);

        var apiHeaderResult = new ModuleApiHeaderGenerator().Generate(scanResult.AllModules, layout.RootPath);
        if (!apiHeaderResult.Success)
        {
            return BuildResult.Fail("Automation test API header generation failed.", apiHeaderResult.Error);
        }

        Console.WriteLine(
            $"  ApiHeaders: generated {apiHeaderResult.GeneratedCount}, unchanged {apiHeaderResult.SkippedCount}");

        var generator = new AutomationTestBuildGenerator();
        var generateResult = generator.Generate(new AutomationTestBuildGenerator.GenerateInput
        {
            ProjectName = layout.ProjectName,
            RootPath = layout.RootPath,
            EngineRoot = scanResult.EngineRoot,
            Modules = scanResult.AllModules,
            TestSources = scanResult.Sources,
            Configuration = BuildConfiguration.Test,
            Platform = "Win64",
        });

        if (!generateResult.Success)
        {
            return BuildResult.Fail("Automation test build target generation failed.", generateResult.Error);
        }

        Directory.CreateDirectory(layout.CMakeSourceDir);
        File.WriteAllText(layout.CMakeListsPath, generateResult.Content);

        return BuildResult.Ok(
            $"Generated automation test build target: {layout.CMakeListsPath} " +
            $"({AutomationTestProfiles.ToCliName(options.Profile)}, " +
            $"{scanResult.Sources.Count} test source(s))");
    }

    /// <summary>
    /// Build AutomationTestRunner and invoke it with the requested list/run filters.
    /// </summary>
    public BuildResult Invoke(
        AutomationTestScanner.ScanResult scanResult,
        AutomationTestOptions options)
    {
        var generateResult = GenerateBuildFiles(scanResult, options, out var layout);
        if (!generateResult.Success)
        {
            return generateResult;
        }

        var configureResult = Configure(layout);
        if (!configureResult.Success)
        {
            return BuildResult.Fail("Automation test CMake configure failed.", configureResult.Output);
        }

        var buildResult = _cmake.Build(
            layout.CMakeBuildDir,
            CMakeBuildConfiguration,
            target: AutomationTestBuildGenerator.RunnerTargetName);
        if (!buildResult.Success)
        {
            return BuildResult.Fail("Automation test runner build failed.", buildResult.Output);
        }

        if (!File.Exists(layout.RunnerPath))
        {
            return BuildResult.Fail(
                "Automation test runner was not produced.",
                $"Expected executable: {layout.RunnerPath}");
        }

        return RunRunner(layout, options);
    }

    private CMakeInvoker.ProcessResult Configure(BuildLayout layout)
    {
        Directory.CreateDirectory(layout.CMakeBuildDir);

        var defines = new Dictionary<string, string>
        {
            ["BUILD_SHARED_LIBS:BOOL"] = "OFF",
        };

        return _cmake.Configure(
            layout.CMakeSourceDir,
            layout.CMakeBuildDir,
            CMakeGeneratorName,
            defines);
    }

    private static BuildResult RunRunner(BuildLayout layout, AutomationTestOptions options)
    {
        var arguments = BuildRunnerArguments(options);
        string argumentString = string.Join(' ', arguments.Select(QuoteArgument));

        Console.WriteLine($"[AutomationTestInvoker] Run: {layout.RunnerPath} {argumentString}");

        var processInfo = new ProcessStartInfo
        {
            FileName = layout.RunnerPath,
            Arguments = argumentString,
            WorkingDirectory = layout.RootPath,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        var output = new StringBuilder();
        using var process = Process.Start(processInfo);
        if (process is null)
        {
            return BuildResult.Fail("Failed to start AutomationTestRunner.", layout.RunnerPath);
        }

        process.OutputDataReceived += (_, eventArgs) =>
        {
            if (eventArgs.Data is null)
            {
                return;
            }

            Console.WriteLine(eventArgs.Data);
            lock (output) { output.AppendLine(eventArgs.Data); }
        };

        process.ErrorDataReceived += (_, eventArgs) =>
        {
            if (eventArgs.Data is null)
            {
                return;
            }

            Console.Error.WriteLine(eventArgs.Data);
            lock (output) { output.AppendLine(eventArgs.Data); }
        };

        process.BeginOutputReadLine();
        process.BeginErrorReadLine();
        process.WaitForExit();

        if (process.ExitCode != 0)
        {
            return BuildResult.Fail(
                $"Automation tests failed with exit code {process.ExitCode}.",
                output.ToString());
        }

        return BuildResult.Ok(
            options.List
                ? "Automation tests listed successfully."
                : "Automation tests completed successfully.");
    }

    private static BuildLayout CreateLayout(AutomationTestScanner.ScanResult scanResult)
    {
        string rootPath = scanResult.EngineMode ? scanResult.EngineRoot : scanResult.ProjectRoot!;
        string projectName = scanResult.EngineMode
            ? "EnigmaEngine"
            : Path.GetFileName(rootPath);
        string cmakeSourceDir = Path.Combine(rootPath, "Intermediate", "AutomationTest");
        string cmakeBuildDir = Path.Combine(cmakeSourceDir, "Build");
        string runnerPath = Path.Combine(
            cmakeBuildDir,
            "Binaries",
            CMakeBuildConfiguration,
            "AutomationTestRunner-Win64-Test.exe");

        return new BuildLayout
        {
            RootPath = rootPath,
            ProjectName = projectName,
            CMakeSourceDir = cmakeSourceDir,
            CMakeBuildDir = cmakeBuildDir,
            CMakeListsPath = Path.Combine(cmakeSourceDir, "CMakeLists.txt"),
            RunnerPath = runnerPath,
        };
    }

    private static List<string> BuildRunnerArguments(AutomationTestOptions options)
    {
        var arguments = new List<string>
        {
            options.List ? "--list" : "--run",
            "--profile",
            AutomationTestProfiles.ToCliName(options.Profile),
        };

        AddOptionalArgument(arguments, "--name", options.Name);
        AddOptionalArgument(arguments, "--name-prefix", options.NamePrefix);
        AddOptionalArgument(arguments, "--module", options.Module);

        foreach (var tag in options.Tags)
        {
            AddOptionalArgument(arguments, "--tag", tag);
        }

        AddOptionalArgument(arguments, "--report", options.ReportDirectory);

        if (options.AllowEmpty)
        {
            arguments.Add("--allow-empty");
        }

        return arguments;
    }

    private static void AddOptionalArgument(List<string> arguments, string name, string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return;
        }

        arguments.Add(name);
        arguments.Add(value);
    }

    private static string QuoteArgument(string argument)
    {
        if (argument.Length == 0)
        {
            return "\"\"";
        }

        return argument.Any(char.IsWhiteSpace)
            ? $"\"{argument.Replace("\"", "\\\"")}\""
            : argument;
    }
}
