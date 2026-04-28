using System.Text.RegularExpressions;
using BuildTool.Models;

namespace BuildTool.Parsers;

/// <summary>
/// Parses .Build.cs files using regex-based extraction.
/// Supports both Add("name") and AddRange(new string[] { "a", "b" }) patterns.
/// Handles multiple calls to the same property and skips commented-out lines.
/// </summary>
public static partial class ModuleParser
{
    // Matches: class ModuleName : ModuleRules
    [GeneratedRegex(@"class\s+(\w+)\s*:\s*ModuleRules", RegexOptions.Compiled)]
    private static partial Regex ClassDeclRegex();

    // Matches: PropertyName.Add("value");  (single add)
    [GeneratedRegex(@"(\w+)\.Add\(\s*""([^""]+)""\s*\)\s*;", RegexOptions.Compiled)]
    private static partial Regex SingleAddRegex();

    // Matches: PropertyName.AddRange(new string[] { "a", "b", "c" });
    // Captures property name and the content inside the braces (may span multiple lines).
    [GeneratedRegex(@"(\w+)\.AddRange\(\s*new\s+string\s*\[\s*\]\s*\{([^}]*)\}\s*\)", RegexOptions.Compiled | RegexOptions.Singleline)]
    private static partial Regex AddRangeRegex();

    // Extracts quoted strings from inside AddRange braces content.
    [GeneratedRegex(@"""([^""]+)""", RegexOptions.Compiled)]
    private static partial Regex QuotedStringRegex();

    // Matches: Type = ModuleType.DeveloperTool;  (property assignment)
    [GeneratedRegex(@"Type\s*=\s*ModuleType\.(\w+)\s*;", RegexOptions.Compiled)]
    private static partial Regex ModuleTypeAssignmentRegex();

    // Known list properties we extract from .Build.cs files.
    private static readonly HashSet<string> KnownListProperties = new(StringComparer.Ordinal)
    {
        "PublicIncludePaths",
        "PrivateIncludePaths",
        "PublicDependencyModuleNames",
        "PrivateDependencyModuleNames",
        "PublicTestDependencyModuleNames",
        "PrivateTestDependencyModuleNames",
        "DynamicallyLoadedModuleNames",
    };

    /// <summary>
    /// Parse a .Build.cs file at the given path.
    /// </summary>
    public static ModuleRules Parse(string filePath)
    {
        var fullPath = Path.GetFullPath(filePath);

        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException($"Module build file not found: {fullPath}", fullPath);
        }

        string source;
        try
        {
            source = File.ReadAllText(fullPath);
        }
        catch (IOException ex)
        {
            throw new ModuleParseException(fullPath, $"Failed to read file: {ex.Message}", ex);
        }

        // Strip single-line comments to avoid matching commented-out code.
        var cleanSource = StripLineComments(source);

        var rules = new ModuleRules { SourceFilePath = fullPath };

        // Extract class name
        var classMatch = ClassDeclRegex().Match(cleanSource);
        if (!classMatch.Success)
        {
            throw new ModuleParseException(fullPath,
                "No class inheriting from 'ModuleRules' found. Expected: class <Name> : ModuleRules");
        }
        rules.ModuleName = classMatch.Groups[1].Value;

        // Collect all list values from Add() and AddRange() calls
        var listValues = ExtractListValues(cleanSource);

        // Map collected values to ModuleRules properties
        if (listValues.TryGetValue("PublicIncludePaths", out var pubInc))
            rules.PublicIncludePaths.AddRange(pubInc);

        if (listValues.TryGetValue("PrivateIncludePaths", out var privInc))
            rules.PrivateIncludePaths.AddRange(privInc);

        if (listValues.TryGetValue("PublicDependencyModuleNames", out var pubDep))
            rules.PublicDependencyModuleNames.AddRange(pubDep);

        if (listValues.TryGetValue("PrivateDependencyModuleNames", out var privDep))
            rules.PrivateDependencyModuleNames.AddRange(privDep);

        if (listValues.TryGetValue("PublicTestDependencyModuleNames", out var pubTestDep))
            rules.PublicTestDependencyModuleNames.AddRange(pubTestDep);

        if (listValues.TryGetValue("PrivateTestDependencyModuleNames", out var privTestDep))
            rules.PrivateTestDependencyModuleNames.AddRange(privTestDep);

        if (listValues.TryGetValue("DynamicallyLoadedModuleNames", out var dynLoad))
            rules.DynamicallyLoadedModuleNames.AddRange(dynLoad);

        // Extract ModuleType assignment (if present)
        var typeMatch = ModuleTypeAssignmentRegex().Match(cleanSource);
        if (typeMatch.Success &&
            Enum.TryParse<ModuleType>(typeMatch.Groups[1].Value, out var moduleType))
        {
            rules.Type = moduleType;
        }

        return rules;
    }

    /// <summary>
    /// Extract all list property values from Add() and AddRange() calls.
    /// </summary>
    private static Dictionary<string, List<string>> ExtractListValues(string source)
    {
        var result = new Dictionary<string, List<string>>(StringComparer.Ordinal);

        // Process AddRange() calls (must be done before Add() to avoid partial matches)
        foreach (Match match in AddRangeRegex().Matches(source))
        {
            var propertyName = match.Groups[1].Value;
            if (!KnownListProperties.Contains(propertyName))
                continue;

            var bracesContent = match.Groups[2].Value;
            var values = ExtractQuotedStrings(bracesContent);

            if (!result.ContainsKey(propertyName))
                result[propertyName] = [];

            result[propertyName].AddRange(values);
        }

        // Process single Add() calls
        foreach (Match match in SingleAddRegex().Matches(source))
        {
            var propertyName = match.Groups[1].Value;
            if (!KnownListProperties.Contains(propertyName))
                continue;

            var value = match.Groups[2].Value;

            if (!result.ContainsKey(propertyName))
                result[propertyName] = [];

            result[propertyName].Add(value);
        }

        return result;
    }

    /// <summary>
    /// Extract all quoted string values from a text fragment.
    /// </summary>
    private static List<string> ExtractQuotedStrings(string text)
    {
        var values = new List<string>();
        foreach (Match match in QuotedStringRegex().Matches(text))
        {
            var value = match.Groups[1].Value;
            if (!string.IsNullOrWhiteSpace(value))
            {
                values.Add(value);
            }
        }
        return values;
    }

    /// <summary>
    /// Remove single-line comments (// ...) to prevent matching commented-out code.
    /// Preserves line structure for error reporting.
    /// </summary>
    private static string StripLineComments(string source)
    {
        // Replace // comments with empty string, but preserve newlines
        return Regex.Replace(source, @"//[^\r\n]*", string.Empty);
    }
}
