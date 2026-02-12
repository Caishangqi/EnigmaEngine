using BuildTool.Parsers;

namespace BuildTool.Tests;

/// <summary>
/// Quick smoke test for ProjectParser.
/// Run via: dotnet run -- test-parse
/// </summary>
public static class ProjectParserTest
{
    public static void Run()
    {
        var testDir = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "Tests");

        Console.WriteLine("=== ProjectParser Smoke Tests ===");
        Console.WriteLine();

        // Test 1: Valid project file
        TestValidProject(Path.Combine(testDir, "TestProject.eproject"));

        // Test 2: Missing FileVersion
        TestMissingVersion(Path.Combine(testDir, "MissingVersion.eproject"));

        // Test 3: Malformed JSON
        TestMalformedJson(Path.Combine(testDir, "Malformed.eproject"));

        // Test 4: Non-existent file
        TestFileNotFound("NonExistent.eproject");

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestValidProject(string path)
    {
        Console.WriteLine("[Test 1] Valid .eproject file");
        var desc = ProjectParser.Parse(path);

        Assert(desc.FileVersion == 1, $"FileVersion: expected 1, got {desc.FileVersion}");
        Assert(desc.EngineAssociation == "0.1.0", $"EngineAssociation: expected '0.1.0', got '{desc.EngineAssociation}'");
        Assert(desc.Modules.Count == 2, $"Modules.Count: expected 2, got {desc.Modules.Count}");
        Assert(desc.Plugins.Count == 2, $"Plugins.Count: expected 2, got {desc.Plugins.Count}");

        var mod0 = desc.Modules[0];
        Assert(mod0.Name == "TestGame", $"Module[0].Name: expected 'TestGame', got '{mod0.Name}'");
        Assert(mod0.Type == Models.EHostType.Runtime, $"Module[0].Type: expected Runtime, got {mod0.Type}");
        Assert(mod0.LoadingPhase == Models.ELoadingPhase.Default, $"Module[0].LoadingPhase: expected Default, got {mod0.LoadingPhase}");
        Assert(mod0.AdditionalDependencies.Count == 2, $"Module[0].AdditionalDependencies.Count: expected 2, got {mod0.AdditionalDependencies.Count}");

        var mod1 = desc.Modules[1];
        Assert(mod1.Name == "TestGameplay", $"Module[1].Name: expected 'TestGameplay', got '{mod1.Name}'");
        Assert(mod1.LoadingPhase == Models.ELoadingPhase.PostEngineInit, $"Module[1].LoadingPhase: expected PostEngineInit, got {mod1.LoadingPhase}");
        Assert(mod1.AdditionalDependencies.Count == 0, $"Module[1].AdditionalDependencies.Count: expected 0, got {mod1.AdditionalDependencies.Count}");

        var plug0 = desc.Plugins[0];
        Assert(plug0.Name == "TestPlugin", $"Plugin[0].Name: expected 'TestPlugin', got '{plug0.Name}'");
        Assert(plug0.Enabled, "Plugin[0].Enabled: expected true");

        var plug1 = desc.Plugins[1];
        Assert(plug1.Name == "DisabledPlugin", $"Plugin[1].Name: expected 'DisabledPlugin', got '{plug1.Name}'");
        Assert(!plug1.Enabled, "Plugin[1].Enabled: expected false");

        Assert(!string.IsNullOrEmpty(desc.SourceFilePath), "SourceFilePath should be set");

        Console.WriteLine("  PASSED");
    }

    private static void TestMissingVersion(string path)
    {
        Console.WriteLine("[Test 2] Missing FileVersion");
        try
        {
            ProjectParser.Parse(path);
            throw new Exception("Expected ProjectParseException but none was thrown.");
        }
        catch (ProjectParseException ex)
        {
            Assert(ex.Message.Contains("FileVersion"), $"Error should mention FileVersion: {ex.Message}");
            Console.WriteLine($"  PASSED (caught: {ex.Message})");
        }
    }

    private static void TestMalformedJson(string path)
    {
        Console.WriteLine("[Test 3] Malformed JSON");
        try
        {
            ProjectParser.Parse(path);
            throw new Exception("Expected ProjectParseException but none was thrown.");
        }
        catch (ProjectParseException ex)
        {
            Assert(ex.Message.Contains("JSON"), $"Error should mention JSON: {ex.Message}");
            Console.WriteLine($"  PASSED (caught: {ex.Message})");
        }
    }

    private static void TestFileNotFound(string path)
    {
        Console.WriteLine("[Test 4] Non-existent file");
        try
        {
            ProjectParser.Parse(path);
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
