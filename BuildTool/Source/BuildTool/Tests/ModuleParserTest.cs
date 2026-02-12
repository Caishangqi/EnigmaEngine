using BuildTool.Parsers;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for ModuleParser.
/// </summary>
public static class ModuleParserTest
{
    public static void Run()
    {
        var testDir = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "Tests");

        Console.WriteLine("=== ModuleParser Smoke Tests ===");
        Console.WriteLine();

        TestValidModule(Path.Combine(testDir, "TestGame.Build.cs"));
        TestEmptyModule(Path.Combine(testDir, "EmptyModule.Build.cs"));
        TestUnrealReference();
        TestFileNotFound();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestValidModule(string path)
    {
        Console.WriteLine("[Test 1] Valid .Build.cs with mixed Add/AddRange");
        var rules = ModuleParser.Parse(path);

        Assert(rules.ModuleName == "TestGame", $"ModuleName: expected 'TestGame', got '{rules.ModuleName}'");

        // PublicIncludePaths: Add("Public")
        Assert(rules.PublicIncludePaths.Count == 1, $"PublicIncludePaths.Count: expected 1, got {rules.PublicIncludePaths.Count}");
        Assert(rules.PublicIncludePaths[0] == "Public", $"PublicIncludePaths[0]: expected 'Public', got '{rules.PublicIncludePaths[0]}'");

        // PrivateIncludePaths: Add("Private")
        Assert(rules.PrivateIncludePaths.Count == 1, $"PrivateIncludePaths.Count: expected 1, got {rules.PrivateIncludePaths.Count}");

        // PublicDependencyModuleNames: AddRange(Core, Engine) + Add(InputCore) = 3
        Assert(rules.PublicDependencyModuleNames.Count == 3, $"PublicDep.Count: expected 3, got {rules.PublicDependencyModuleNames.Count}");
        Assert(rules.PublicDependencyModuleNames.Contains("Core"), "PublicDep should contain 'Core'");
        Assert(rules.PublicDependencyModuleNames.Contains("Engine"), "PublicDep should contain 'Engine'");
        Assert(rules.PublicDependencyModuleNames.Contains("InputCore"), "PublicDep should contain 'InputCore'");

        // PrivateDependencyModuleNames: AddRange(Slate, SlateCore) = 2
        // "CommentedOut" should NOT be present (it's in a comment)
        Assert(rules.PrivateDependencyModuleNames.Count == 2, $"PrivateDep.Count: expected 2, got {rules.PrivateDependencyModuleNames.Count}");
        Assert(!rules.PrivateDependencyModuleNames.Contains("CommentedOut"), "PrivateDep should NOT contain 'CommentedOut'");

        // DynamicallyLoadedModuleNames: Add(OnlineSubsystem) = 1
        Assert(rules.DynamicallyLoadedModuleNames.Count == 1, $"DynLoad.Count: expected 1, got {rules.DynamicallyLoadedModuleNames.Count}");

        Assert(!string.IsNullOrEmpty(rules.SourceFilePath), "SourceFilePath should be set");

        Console.WriteLine("  PASSED");
    }

    private static void TestEmptyModule(string path)
    {
        Console.WriteLine("[Test 2] Empty module (no dependencies)");
        var rules = ModuleParser.Parse(path);

        Assert(rules.ModuleName == "EmptyModule", $"ModuleName: expected 'EmptyModule', got '{rules.ModuleName}'");
        Assert(rules.PublicDependencyModuleNames.Count == 0, $"PublicDep.Count: expected 0, got {rules.PublicDependencyModuleNames.Count}");
        Assert(rules.PrivateDependencyModuleNames.Count == 0, $"PrivateDep.Count: expected 0, got {rules.PrivateDependencyModuleNames.Count}");

        Console.WriteLine("  PASSED");
    }

    private static void TestUnrealReference()
    {
        Console.WriteLine("[Test 3] Unreal reference .Build.cs (multi-AddRange)");
        var path = Path.GetFullPath(Path.Combine(
            AppContext.BaseDirectory, "..", "..", "..", "..", "..", "..",
            "Reference", "@example_unreal_project", "Source", "Kila_Hourbound", "Kila_Hourbound.Build.cs"));

        if (!File.Exists(path))
        {
            Console.WriteLine("  SKIPPED (reference file not found)");
            return;
        }

        var rules = ModuleParser.Parse(path);

        Assert(rules.ModuleName == "Kila_Hourbound", $"ModuleName: expected 'Kila_Hourbound', got '{rules.ModuleName}'");

        // Multiple AddRange calls for PublicDependencyModuleNames
        Assert(rules.PublicDependencyModuleNames.Count > 5, $"PublicDep.Count: expected >5, got {rules.PublicDependencyModuleNames.Count}");
        Assert(rules.PublicDependencyModuleNames.Contains("Core"), "PublicDep should contain 'Core'");
        Assert(rules.PublicDependencyModuleNames.Contains("BuffModule"), "PublicDep should contain 'BuffModule'");

        // PrivateDependencyModuleNames
        Assert(rules.PrivateDependencyModuleNames.Count >= 2, $"PrivateDep.Count: expected >=2, got {rules.PrivateDependencyModuleNames.Count}");

        // PublicIncludePaths with Path.Combine expressions
        Assert(rules.PublicIncludePaths.Count >= 1, $"PublicIncludePaths.Count: expected >=1, got {rules.PublicIncludePaths.Count}");

        // "OnlineSubsystem" is commented out, should NOT be in PrivateDep
        Assert(!rules.PrivateDependencyModuleNames.Contains("OnlineSubsystem"), "PrivateDep should NOT contain commented-out 'OnlineSubsystem'");

        Console.WriteLine($"  PASSED (PublicDep={rules.PublicDependencyModuleNames.Count}, PrivateDep={rules.PrivateDependencyModuleNames.Count})");
    }

    private static void TestFileNotFound()
    {
        Console.WriteLine("[Test 4] Non-existent file");
        try
        {
            ModuleParser.Parse("NonExistent.Build.cs");
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
