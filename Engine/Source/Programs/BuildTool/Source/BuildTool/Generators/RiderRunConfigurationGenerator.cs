// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Generators;

using System.Xml.Linq;
using BuildTool.Utils;

/// <summary>
/// Generates shared Rider .run configurations without writing to .idea.
/// </summary>
public sealed class RiderRunConfigurationGenerator
{
    public sealed class GenerateInput
    {
        public required string OutputDirectory { get; init; }
        public required string BuildToolProjectPath { get; init; }
        public required string WorkingDirectory { get; init; }
        public required string RootArgument { get; init; }
        public bool EngineMode { get; init; }
        public string ReportDirectory { get; init; } = "Saved/AutomationReports";
    }

    public sealed class GenerateResult
    {
        public required bool Success { get; init; }
        public string? Error { get; init; }
        public int GeneratedCount { get; init; }
        public IReadOnlyList<string> OutputPaths { get; init; } = [];
    }

    public GenerateResult GenerateAutomationTestConfigs(GenerateInput input)
    {
        try
        {
            string runDirectory = Path.Combine(input.OutputDirectory, ".run");
            Directory.CreateDirectory(runDirectory);

            var outputPaths = new List<string>();
            WriteConfig(input, runDirectory, "Enigma Automation Tests - Fast",
                $"automation-test {Quote(input.RootArgument)}{BuildEngineSwitch(input)} --run --profile local-fast --report {Quote(input.ReportDirectory)}",
                outputPaths);
            WriteConfig(input, runDirectory, "Enigma Automation Tests - Filtered",
                $"automation-test {Quote(input.RootArgument)}{BuildEngineSwitch(input)} --run --profile local-fast --name-prefix System --allow-empty --report {Quote(input.ReportDirectory)}",
                outputPaths);
            WriteConfig(input, runDirectory, "Enigma Automation Tests - List",
                $"automation-test {Quote(input.RootArgument)}{BuildEngineSwitch(input)} --list --profile all-non-perf",
                outputPaths);

            return new GenerateResult
            {
                Success = true,
                GeneratedCount = outputPaths.Count,
                OutputPaths = outputPaths,
            };
        }
        catch (Exception ex)
        {
            return new GenerateResult
            {
                Success = false,
                Error = ex.Message,
            };
        }
    }

    private static void WriteConfig(
        GenerateInput input,
        string runDirectory,
        string name,
        string programArguments,
        List<string> outputPaths)
    {
        string fileName = SanitizeFileName(name) + ".run.xml";
        string outputPath = Path.Combine(runDirectory, fileName);

        var document = new XDocument(
            new XElement("component",
                new XAttribute("name", "ProjectRunConfigurationManager"),
                new XElement("configuration",
                    new XAttribute("default", "false"),
                    new XAttribute("name", name),
                    new XAttribute("type", "DotNetProject"),
                    new XAttribute("factoryName", ".NET Project"),
                    new XElement("option",
                        new XAttribute("name", "PROJECT_PATH"),
                        new XAttribute("value", input.BuildToolProjectPath)),
                    new XElement("option",
                        new XAttribute("name", "PROJECT_TFM"),
                        new XAttribute("value", "net9.0")),
                    new XElement("option",
                        new XAttribute("name", "PROJECT_ARGUMENTS"),
                        new XAttribute("value", programArguments)),
                    new XElement("option",
                        new XAttribute("name", "PROGRAM_PARAMETERS"),
                        new XAttribute("value", programArguments)),
                    new XElement("option",
                        new XAttribute("name", "WORKING_DIRECTORY"),
                        new XAttribute("value", input.WorkingDirectory)),
                    new XElement("option",
                        new XAttribute("name", "PROJECT_WORKING_DIRECTORY"),
                        new XAttribute("value", input.WorkingDirectory)),
                    new XElement("option",
                        new XAttribute("name", "PROJECT_KIND"),
                        new XAttribute("value", "DotNetCore")),
                    new XElement("method",
                        new XAttribute("v", "2"),
                        new XElement("option",
                            new XAttribute("name", "Build"))))));

        AtomicFileWriter.WriteIfChanged(outputPath, document.ToString());
        outputPaths.Add(outputPath);
    }

    private static string BuildEngineSwitch(GenerateInput input)
    {
        return input.EngineMode ? " --engine" : string.Empty;
    }

    private static string Quote(string value)
    {
        return value.Any(char.IsWhiteSpace)
            ? $"\"{value.Replace("\"", "\\\"")}\""
            : value;
    }

    private static string SanitizeFileName(string value)
    {
        foreach (char invalid in Path.GetInvalidFileNameChars())
        {
            value = value.Replace(invalid, '_');
        }

        return value;
    }
}
