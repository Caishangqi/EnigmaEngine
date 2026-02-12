using System.Text.Json.Nodes;
using BuildTool.Scaffolding;

namespace BuildTool.Tests;

/// <summary>
/// Unit tests for <see cref="EprojectModifier"/>.
/// </summary>
public static class EprojectModifierTest
{
    public static void Run()
    {
        Console.WriteLine("=== EprojectModifier Tests ===");
        Console.WriteLine();

        TestAddModuleToEmptyArray();
        TestAddModuleToExistingArray();
        TestAddPluginToEmptyArray();
        TestAddPluginToExistingArray();
        TestPreservesExistingContent();
        TestModuleEntryFormat();
        TestPluginEntryFormat();
        TestRemoveModule();
        TestRemovePlugin();
        TestRemoveNonexistent();
        TestInvalidEprojectFile();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestAddModuleToEmptyArray()
    {
        Console.WriteLine("[Test 1] AddModule to empty Modules array");

        var path = CreateTempEproject(modules: "[]");
        try
        {
            var result = EprojectModifier.AddModule(path, "NewModule");
            Assert(result.Success, $"Expected success: {result.Message}");

            var root = ParseJson(path);
            var modules = root["Modules"]!.AsArray();
            Assert(modules.Count == 1, $"Expected 1 module, got {modules.Count}");
            Assert(modules[0]!["Name"]!.GetValue<string>() == "NewModule", "Name mismatch");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestAddModuleToExistingArray()
    {
        Console.WriteLine("[Test 2] AddModule appends to existing array");

        var path = CreateTempEproject();
        try
        {
            var result = EprojectModifier.AddModule(path, "ExtraModule");
            Assert(result.Success, $"Expected success: {result.Message}");

            var modules = ParseJson(path)["Modules"]!.AsArray();
            Assert(modules.Count == 3, $"Expected 3 modules, got {modules.Count}");
            Assert(modules[2]!["Name"]!.GetValue<string>() == "ExtraModule", "Appended name mismatch");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestAddPluginToEmptyArray()
    {
        Console.WriteLine("[Test 3] AddPlugin to empty Plugins array");

        var path = CreateTempEproject(plugins: "[]");
        try
        {
            var result = EprojectModifier.AddPlugin(path, "NewPlugin");
            Assert(result.Success, $"Expected success: {result.Message}");

            var plugins = ParseJson(path)["Plugins"]!.AsArray();
            Assert(plugins.Count == 1, $"Expected 1 plugin, got {plugins.Count}");
            Assert(plugins[0]!["Name"]!.GetValue<string>() == "NewPlugin", "Name mismatch");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestAddPluginToExistingArray()
    {
        Console.WriteLine("[Test 4] AddPlugin appends to existing array");

        var path = CreateTempEproject();
        try
        {
            var result = EprojectModifier.AddPlugin(path, "ExtraPlugin", enabled: false);
            Assert(result.Success, $"Expected success: {result.Message}");

            var plugins = ParseJson(path)["Plugins"]!.AsArray();
            Assert(plugins.Count == 3, $"Expected 3 plugins, got {plugins.Count}");
            Assert(plugins[2]!["Name"]!.GetValue<string>() == "ExtraPlugin", "Appended name mismatch");
            Assert(plugins[2]!["Enabled"]!.GetValue<bool>() == false, "Enabled should be false");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestPreservesExistingContent()
    {
        Console.WriteLine("[Test 5] Preserves existing content (FileVersion, EngineAssociation)");

        var path = CreateTempEproject();
        try
        {
            EprojectModifier.AddModule(path, "Extra");

            var root = ParseJson(path);
            Assert(root["FileVersion"]!.GetValue<int>() == 1, "FileVersion should be preserved");
            Assert(root["EngineAssociation"]!.GetValue<string>() == "0.1.0", "EngineAssociation should be preserved");

            var modules = root["Modules"]!.AsArray();
            Assert(modules[0]!["Name"]!.GetValue<string>() == "TestGame", "First module should be preserved");
            Assert(modules[1]!["Name"]!.GetValue<string>() == "TestGameplay", "Second module should be preserved");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestModuleEntryFormat()
    {
        Console.WriteLine("[Test 6] Module entry has correct JSON format");

        var path = CreateTempEproject(modules: "[]");
        try
        {
            EprojectModifier.AddModule(path, "MyMod", "Editor", "PostEngineInit");

            var entry = ParseJson(path)["Modules"]!.AsArray()[0]!;
            Assert(entry["Name"]!.GetValue<string>() == "MyMod", "Name mismatch");
            Assert(entry["Type"]!.GetValue<string>() == "Editor", "Type mismatch");
            Assert(entry["LoadingPhase"]!.GetValue<string>() == "PostEngineInit", "LoadingPhase mismatch");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestPluginEntryFormat()
    {
        Console.WriteLine("[Test 7] Plugin entry has correct JSON format");

        var path = CreateTempEproject(plugins: "[]");
        try
        {
            EprojectModifier.AddPlugin(path, "MyPlug", enabled: true);

            var entry = ParseJson(path)["Plugins"]!.AsArray()[0]!;
            Assert(entry["Name"]!.GetValue<string>() == "MyPlug", "Name mismatch");
            Assert(entry["Enabled"]!.GetValue<bool>() == true, "Enabled mismatch");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestRemoveModule()
    {
        Console.WriteLine("[Test 8] RemoveModule removes entry");

        var path = CreateTempEproject();
        try
        {
            var result = EprojectModifier.RemoveModule(path, "TestGame");
            Assert(result.Success, $"Expected success: {result.Message}");

            var modules = ParseJson(path)["Modules"]!.AsArray();
            Assert(modules.Count == 1, $"Expected 1 module remaining, got {modules.Count}");
            Assert(modules[0]!["Name"]!.GetValue<string>() == "TestGameplay", "Wrong module removed");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestRemovePlugin()
    {
        Console.WriteLine("[Test 9] RemovePlugin removes entry");

        var path = CreateTempEproject();
        try
        {
            var result = EprojectModifier.RemovePlugin(path, "DisabledPlugin");
            Assert(result.Success, $"Expected success: {result.Message}");

            var plugins = ParseJson(path)["Plugins"]!.AsArray();
            Assert(plugins.Count == 1, $"Expected 1 plugin remaining, got {plugins.Count}");
            Assert(plugins[0]!["Name"]!.GetValue<string>() == "TestPlugin", "Wrong plugin removed");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestRemoveNonexistent()
    {
        Console.WriteLine("[Test 10] Remove nonexistent entry returns Fail");

        var path = CreateTempEproject();
        try
        {
            var result = EprojectModifier.RemoveModule(path, "DoesNotExist");
            Assert(!result.Success, "Should fail for nonexistent module");
            Assert(result.Message.Contains("not found", StringComparison.OrdinalIgnoreCase),
                $"Error should mention 'not found': {result.Message}");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestInvalidEprojectFile()
    {
        Console.WriteLine("[Test 11] Invalid .eproject file returns Fail");

        var path = Path.Combine(Path.GetTempPath(), $"invalid_{Guid.NewGuid():N}.eproject");
        try
        {
            File.WriteAllText(path, "{ not valid json }}}");
            var result = EprojectModifier.AddModule(path, "Test");
            Assert(!result.Success, "Should fail for invalid .eproject");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    // --- Helpers ---

    private static string CreateTempEproject(
        string? modules = null,
        string? plugins = null)
    {
        modules ??= """
            [
                { "Name": "TestGame", "Type": "Runtime", "LoadingPhase": "Default", "AdditionalDependencies": ["Engine","Core"] },
                { "Name": "TestGameplay", "Type": "Runtime", "LoadingPhase": "PostEngineInit" }
            ]
            """;
        plugins ??= """
            [
                { "Name": "TestPlugin", "Enabled": true },
                { "Name": "DisabledPlugin", "Enabled": false }
            ]
            """;

        var json = $$"""
            {
                "FileVersion": 1,
                "EngineAssociation": "0.1.0",
                "Modules": {{modules}},
                "Plugins": {{plugins}}
            }
            """;

        var path = Path.Combine(Path.GetTempPath(), $"test_{Guid.NewGuid():N}.eproject");
        File.WriteAllText(path, json);
        return path;
    }

    private static JsonObject ParseJson(string path) =>
        JsonNode.Parse(File.ReadAllText(path))!.AsObject();

    private static void DeleteTemp(string path)
    {
        try { if (File.Exists(path)) File.Delete(path); }
        catch { /* Best effort */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}