// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

using System.Text.RegularExpressions;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Utils;

/// <summary>
/// Modifies .Target.cs files by adding/removing <c>ExtraModuleNames.Add()</c> lines.
/// </summary>
public static partial class TargetFileModifier
{
    /// <summary>Matches an ExtraModuleNames.Add("...") line with optional surrounding whitespace.</summary>
    [GeneratedRegex(@"^([ \t]*)ExtraModuleNames\.Add\(\s*""([^""]*)""\s*\)\s*;", RegexOptions.Multiline)]
    private static partial Regex ExtraModuleAddRegex();

    /// <summary>Matches the constructor closing brace (first <c>}</c> after class body opens).</summary>
    [GeneratedRegex(@"^([ \t]*)\}", RegexOptions.Multiline)]
    private static partial Regex ClosingBraceRegex();

    /// <summary>
    /// Add an <c>ExtraModuleNames.Add("{moduleName}")</c> line to the target file.
    /// Inserts after the last existing Add() line, or before the constructor closing brace if none exist.
    /// </summary>
    public static BuildResult AddModule(string targetFilePath, string moduleName)
    {
        try
        {
            TargetParser.Parse(targetFilePath);
        }
        catch (Exception ex) when (ex is FileNotFoundException or TargetParseException)
        {
            return BuildResult.Fail(ex.Message);
        }

        try
        {
            var content = File.ReadAllText(targetFilePath);
            var lineEnding = content.Contains("\r\n") ? "\r\n" : "\n";

            // Check if module already exists
            var addMatches = ExtraModuleAddRegex().Matches(content);
            foreach (Match m in addMatches)
            {
                if (string.Equals(m.Groups[2].Value, moduleName, StringComparison.Ordinal))
                    return BuildResult.Fail($"Module '{moduleName}' already exists in {Path.GetFileName(targetFilePath)}.");
            }

            string newContent;

            if (addMatches.Count > 0)
            {
                // Insert after the last ExtraModuleNames.Add() line
                var lastMatch = addMatches[^1];
                var indent = lastMatch.Groups[1].Value;
                var insertPos = lastMatch.Index + lastMatch.Length;

                // Skip to end of line (past any trailing whitespace/newline)
                while (insertPos < content.Length && content[insertPos] != '\n' && content[insertPos] != '\r')
                    insertPos++;
                if (insertPos < content.Length && content[insertPos] == '\r')
                    insertPos++;
                if (insertPos < content.Length && content[insertPos] == '\n')
                    insertPos++;

                var newLine = $"{indent}ExtraModuleNames.Add(\"{moduleName}\");{lineEnding}";
                newContent = content.Insert(insertPos, newLine);
            }
            else
            {
                // No existing Add() lines - find the second closing brace (constructor's })
                // The first } closes the constructor, the second } closes the class
                var braceMatches = ClosingBraceRegex().Matches(content);
                if (braceMatches.Count < 2)
                    return BuildResult.Fail($"Could not find constructor closing brace in {Path.GetFileName(targetFilePath)}.");

                // First closing brace is the constructor's
                var constructorBrace = braceMatches[0];
                var indent = constructorBrace.Groups[1].Value + "    ";
                var newLine = $"{indent}ExtraModuleNames.Add(\"{moduleName}\");{lineEnding}";
                newContent = content.Insert(constructorBrace.Index, newLine);
            }

            var status = AtomicFileWriter.WriteIfChanged(targetFilePath, newContent);
            return status == AtomicFileWriter.WriteStatus.Error
                ? BuildResult.Fail($"Failed to write {targetFilePath}")
                : BuildResult.Ok($"Module '{moduleName}' added to {Path.GetFileName(targetFilePath)}.");
        }
        catch (IOException ex)
        {
            return BuildResult.Fail(ex.Message);
        }
    }

    /// <summary>
    /// Remove the <c>ExtraModuleNames.Add("{moduleName}")</c> line from the target file.
    /// </summary>
    public static BuildResult RemoveModule(string targetFilePath, string moduleName)
    {
        try
        {
            TargetParser.Parse(targetFilePath);
        }
        catch (Exception ex) when (ex is FileNotFoundException or TargetParseException)
        {
            return BuildResult.Fail(ex.Message);
        }

        try
        {
            var content = File.ReadAllText(targetFilePath);

            // Build a specific regex for this module name
            var pattern = $@"[ \t]*ExtraModuleNames\.Add\(\s*""{Regex.Escape(moduleName)}""\s*\)\s*;\s*(\r?\n)?";
            var match = Regex.Match(content, pattern);

            if (!match.Success)
                return BuildResult.Fail($"Module '{moduleName}' not found in {Path.GetFileName(targetFilePath)}.");

            var newContent = content.Remove(match.Index, match.Length);

            var status = AtomicFileWriter.WriteIfChanged(targetFilePath, newContent);
            return status == AtomicFileWriter.WriteStatus.Error
                ? BuildResult.Fail($"Failed to write {targetFilePath}")
                : BuildResult.Ok($"Module '{moduleName}' removed from {Path.GetFileName(targetFilePath)}.");
        }
        catch (IOException ex)
        {
            return BuildResult.Fail(ex.Message);
        }
    }
}
