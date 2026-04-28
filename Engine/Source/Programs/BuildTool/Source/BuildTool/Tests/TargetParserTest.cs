using BuildTool.Parsers;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for TargetParser.
/// </summary>
public static class TargetParserTest
{
    public static void Run()
    {
        var testDir = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "Tests");

        Console.WriteLine("=== TargetParser Smoke Tests ===");
        Console.WriteLine();

        TestValidTarget(Path.Combine(testDir, "TestGame.Target.cs"));
        TestNoExtraModules(Path.Combine(testDir, "Empty.Target.cs"));
        TestUnrealReference();
        TestFileNotFound();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestValidTarget(string path)
    {
        Console.WriteLine("[Test 1] Valid .Target.cs file");
        var rules = TargetParser.Parse(path);

        Assert(rules.TargetName == "TestGameTarget", $"TargetName: expected 'TestGameTarget', got '{rules.TargetName}'");
        Assert(rules.Type == Models.TargetType.Game, $"Type: expected Game, got {rules.Type}");
        Assert(rules.DefaultBuildSettings == "V2", $"DefaultBuildSettings: expected 'V2', got '{rules.DefaultBuildSettings}'");
        Assert(rules.ExtraModuleNames.Count == 3, $"ExtraModuleNames.Count: expected 3, got {rules.ExtraModuleNames.Count}");
        Assert(rules.ExtraModuleNames[0] == "TestGame", $"ExtraModuleNames[0]: expected 'TestGame', got '{rules.ExtraModuleNames[0]}'");
        Assert(rules.ExtraModuleNames[1] == "TestGameplay", $"ExtraModuleNames[1]: expected 'TestGameplay', got '{rules.ExtraModuleNames[1]}'");
        Assert(rules.ExtraModuleNames[2] == "CoreUtils", $"ExtraModuleNames[2]: expected 'CoreUtils', got '{rules.ExtraModuleNames[2]}'");
        Assert(!string.IsNullOrEmpty(rules.SourceFilePath), "SourceFilePath should be set");

        Console.WriteLine("  PASSED");
    }

    private static void TestNoExtraModules(string path)
    {
        Console.WriteLine("[Test 2] Missing ExtraModuleNames");
        try
        {
            TargetParser.Parse(path);
            throw new Exception("Expected TargetParseException but none was thrown.");
        }
        catch (TargetParseException ex)
        {
            Assert(ex.Message.Contains("ExtraModuleNames"), $"Error should mention ExtraModuleNames: {ex.Message}");
            Console.WriteLine($"  PASSED (caught: {ex.Message})");
        }
    }

    private static void TestUnrealReference()
    {
        Console.WriteLine("[Test 3] Unreal reference .Target.cs (multi-module)");
        var path = Path.GetFullPath(Path.Combine(
            AppContext.BaseDirectory, "..", "..", "..", "..", "..", "..", "..", "..", "..",
            ".reference", "@example_unreal_project", "Source", "Kila_Hourbound.Target.cs"));

        if (!File.Exists(path))
        {
            Console.WriteLine("  SKIPPED (reference file not found)");
            return;
        }

        var rules = TargetParser.Parse(path);

        Assert(rules.TargetName == "Kila_HourboundTarget", $"TargetName: expected 'Kila_HourboundTarget', got '{rules.TargetName}'");
        Assert(rules.Type == Models.TargetType.Game, $"Type: expected Game, got {rules.Type}");
        Assert(rules.DefaultBuildSettings == "V5", $"DefaultBuildSettings: expected 'V5', got '{rules.DefaultBuildSettings}'");
        Assert(rules.ExtraModuleNames.Count == 5, $"ExtraModuleNames.Count: expected 5, got {rules.ExtraModuleNames.Count}");
        Assert(rules.ExtraModuleNames[0] == "Kila_Hourbound", $"ExtraModuleNames[0]: expected 'Kila_Hourbound', got '{rules.ExtraModuleNames[0]}'");

        Console.WriteLine("  PASSED");
    }

    private static void TestFileNotFound()
    {
        Console.WriteLine("[Test 4] Non-existent file");
        try
        {
            TargetParser.Parse("NonExistent.Target.cs");
            throw new Exception("Expected FileNotFoundException but none was thrown.");
        }
        catch (FileNotFoundException)
        {
            Console.WriteLine("  PASSED (caught FileNotFoundException)");
        }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception($"Assertion failed: {message}");
        }
    }
}
