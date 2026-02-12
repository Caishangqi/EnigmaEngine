using System.Text.RegularExpressions;
using BuildTool.Models;

namespace BuildTool.Parsers;

/// <summary>
/// Parses .Target.cs files using regex-based extraction.
/// Extracts TargetType, DefaultBuildSettings, and ExtraModuleNames
/// from simplified C# syntax without requiring Roslyn compilation.
/// </summary>
public static partial class TargetParser
{
    // Matches: class FooTarget : TargetRules
    [GeneratedRegex(@"class\s+(\w+)\s*:\s*TargetRules", RegexOptions.Compiled)]
    private static partial Regex ClassDeclRegex();

    // Matches: Type = TargetType.Game;
    [GeneratedRegex(@"Type\s*=\s*TargetType\.(\w+)\s*;", RegexOptions.Compiled)]
    private static partial Regex TargetTypeRegex();

    // Matches: DefaultBuildSettings = BuildSettingsVersion.V5;  or  DefaultBuildSettings = "something";
    [GeneratedRegex(@"DefaultBuildSettings\s*=\s*(?:BuildSettingsVersion\.)?(\w+)\s*;", RegexOptions.Compiled)]
    private static partial Regex DefaultBuildSettingsRegex();

    // Matches: ExtraModuleNames.Add("ModuleName");
    [GeneratedRegex(@"ExtraModuleNames\.Add\(\s*""([^""]+)""\s*\)\s*;", RegexOptions.Compiled)]
    private static partial Regex ExtraModuleAddRegex();

    /// <summary>
    /// Parse a .Target.cs file at the given path.
    /// </summary>
    /// <param name="filePath">Absolute or relative path to the .Target.cs file.</param>
    /// <returns>A populated <see cref="TargetRules"/> instance.</returns>
    /// <exception cref="FileNotFoundException">The file does not exist.</exception>
    /// <exception cref="TargetParseException">The file cannot be parsed or is missing required fields.</exception>
    public static TargetRules Parse(string filePath)
    {
        var fullPath = Path.GetFullPath(filePath);

        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException($"Target file not found: {fullPath}", fullPath);
        }

        string source;
        try
        {
            source = File.ReadAllText(fullPath);
        }
        catch (IOException ex)
        {
            throw new TargetParseException(fullPath, $"Failed to read file: {ex.Message}", ex);
        }

        var rules = new TargetRules { SourceFilePath = fullPath };

        // Extract class name
        var classMatch = ClassDeclRegex().Match(source);
        if (!classMatch.Success)
        {
            throw new TargetParseException(fullPath,
                "No class inheriting from 'TargetRules' found. Expected: class <Name>Target : TargetRules");
        }
        rules.TargetName = classMatch.Groups[1].Value;

        // Extract TargetType
        var typeMatch = TargetTypeRegex().Match(source);
        if (typeMatch.Success)
        {
            if (!Enum.TryParse<TargetType>(typeMatch.Groups[1].Value, ignoreCase: true, out var targetType))
            {
                throw new TargetParseException(fullPath,
                    $"Unsupported TargetType: '{typeMatch.Groups[1].Value}'. Supported: Game.");
            }
            rules.Type = targetType;
        }

        // Extract DefaultBuildSettings
        var settingsMatch = DefaultBuildSettingsRegex().Match(source);
        if (settingsMatch.Success)
        {
            rules.DefaultBuildSettings = settingsMatch.Groups[1].Value;
        }

        // Extract ExtraModuleNames
        foreach (Match addMatch in ExtraModuleAddRegex().Matches(source))
        {
            var moduleName = addMatch.Groups[1].Value;
            if (!string.IsNullOrWhiteSpace(moduleName))
            {
                rules.ExtraModuleNames.Add(moduleName);
            }
        }

        Validate(rules, fullPath);

        return rules;
    }

    /// <summary>
    /// Validate semantic constraints on the parsed target rules.
    /// </summary>
    private static void Validate(TargetRules rules, string filePath)
    {
        if (rules.ExtraModuleNames.Count == 0)
        {
            throw new TargetParseException(filePath,
                "No ExtraModuleNames found. At least one ExtraModuleNames.Add(\"...\") call is required.");
        }
    }
}
