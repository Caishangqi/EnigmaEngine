using BuildTool.Models;
using BuildTool.Parsers;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for PluginParser.
/// Validates:
///   [1]  Full .eplugin parsed: all metadata fields mapped
///   [2]  FileVersion parsed correctly
///   [3]  Version and VersionName parsed
///   [4]  FriendlyName, Description, Category parsed
///   [5]  CreatedBy and CreatedByURL parsed
///   [6]  CanContainContent parsed (true)
///   [7]  Modules array: correct count
///   [8]  Module[0]: Name, Type, LoadingPhase mapped
///   [9]  Module[1]: Editor type with Default phase
///   [10] Plugins array: correct count
///   [11] Plugin[0]: Name and Enabled=true
///   [12] Plugin[1]: Name and Enabled=false
///   [13] SourceFilePath set after parse
///   [14] Minimal .eplugin: optional fields default gracefully
///   [15] Minimal: Modules and Plugins default to empty lists
///   [16] Minimal: CanContainContent defaults to false
///   [17] Minimal: Version defaults to 1
///   [18] Missing FileVersion throws PluginParseException
///   [19] Missing FriendlyName throws PluginParseException
///   [20] Empty module name throws PluginParseException
///   [21] Malformed JSON throws PluginParseException
///   [22] Non-existent file throws FileNotFoundException
/// </summary>
public static class PluginParserTest
{
    private static int _passed;
    private static int _failed;

    public static void Run()
    {
        _passed = 0;
        _failed = 0;

        Console.WriteLine("=== PluginParser Smoke Tests ===");
        Console.WriteLine();

        var testDir = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "Tests");

        TestFullPlugin(Path.Combine(testDir, "TestArcade.eplugin"));
        TestMinimalPlugin(Path.Combine(testDir, "TestMinimal.eplugin"));
        TestMissingFileVersion();
        TestMissingFriendlyName();
        TestEmptyModuleName();
        TestMalformedJson();
        TestFileNotFound();

        Console.WriteLine();
        Console.WriteLine($"=== {_passed}/{_passed + _failed} tests passed ===");

        if (_failed > 0)
            throw new Exception($"PluginParserTest: {_failed} test(s) failed.");
    }

    /// <summary>Tests [1]-[13]: Full .eplugin with all fields.</summary>
    private static void TestFullPlugin(string path)
    {
        Console.WriteLine("[Tests 1-13] Full .eplugin (TestArcade)");

        var desc = PluginParser.Parse(path);

        Check(desc != null,
            "[1]  Full .eplugin parsed successfully");

        Check(desc.FileVersion == 1,
            "[2]  FileVersion = 1");

        Check(desc.Version == 3 && desc.VersionName == "1.2.0",
            "[3]  Version = 3, VersionName = \"1.2.0\"");

        Check(desc.FriendlyName == "Arcade Feature"
           && desc.Description == "Feature plugin for arcade gameplay mechanics"
           && desc.Category == "Gameplay",
            "[4]  FriendlyName, Description, Category parsed");

        Check(desc.CreatedBy == "EnigmaEngine"
           && desc.CreatedByURL == "https://enigmaengine.com",
            "[5]  CreatedBy and CreatedByURL parsed");

        Check(desc.CanContainContent == true,
            "[6]  CanContainContent = true");

        Check(desc.Modules.Count == 2,
            "[7]  Modules array: count = 2");

        Check(desc.Modules[0].Name == "ArcadeFeature"
           && desc.Modules[0].Type == EHostType.Runtime
           && desc.Modules[0].LoadingPhase == ELoadingPhase.PostEngineInit,
            "[8]  Module[0]: ArcadeFeature, Runtime, PostEngineInit");

        Check(desc.Modules[1].Name == "ArcadeFeatureEditor"
           && desc.Modules[1].Type == EHostType.Editor
           && desc.Modules[1].LoadingPhase == ELoadingPhase.Default,
            "[9]  Module[1]: ArcadeFeatureEditor, Editor, Default");

        Check(desc.Plugins.Count == 2,
            "[10] Plugins array: count = 2");

        Check(desc.Plugins[0].Name == "CoreUtils" && desc.Plugins[0].Enabled == true,
            "[11] Plugin[0]: CoreUtils, Enabled=true");

        Check(desc.Plugins[1].Name == "InputSystem" && desc.Plugins[1].Enabled == false,
            "[12] Plugin[1]: InputSystem, Enabled=false");

        Check(!string.IsNullOrEmpty(desc.SourceFilePath)
           && desc.SourceFilePath.Contains("TestArcade.eplugin"),
            "[13] SourceFilePath set after parse");
    }

    /// <summary>Tests [14]-[17]: Minimal .eplugin with only required fields.</summary>
    private static void TestMinimalPlugin(string path)
    {
        Console.WriteLine("[Tests 14-17] Minimal .eplugin (TestMinimal)");

        var desc = PluginParser.Parse(path);

        Check(desc.FriendlyName == "Empty Plugin"
           && desc.Description == "A minimal plugin with no modules or dependencies"
           && desc.Category == string.Empty
           && desc.CreatedBy == string.Empty
           && desc.CreatedByURL == string.Empty,
            "[14] Optional fields default gracefully");

        Check(desc.Modules.Count == 0 && desc.Plugins.Count == 0,
            "[15] Modules and Plugins default to empty lists");

        Check(desc.CanContainContent == false,
            "[16] CanContainContent defaults to false");

        Check(desc.Version == 1,
            "[17] Version defaults to 1");
    }

    /// <summary>Test [18]: Missing FileVersion.</summary>
    private static void TestMissingFileVersion()
    {
        Console.WriteLine("[Test 18] Missing FileVersion");

        var tmpFile = WriteTempJson("{\"FriendlyName\": \"Test\"}");
        try
        {
            PluginParser.Parse(tmpFile);
            Check(false, "[18] Missing FileVersion throws PluginParseException");
        }
        catch (PluginParseException ex) when (ex.Message.Contains("FileVersion"))
        {
            Check(true, "[18] Missing FileVersion throws PluginParseException");
        }
        finally { TryDelete(tmpFile); }
    }

    /// <summary>Test [19]: Missing FriendlyName.</summary>
    private static void TestMissingFriendlyName()
    {
        Console.WriteLine("[Test 19] Missing FriendlyName");

        var tmpFile = WriteTempJson("{\"FileVersion\": 1}");
        try
        {
            PluginParser.Parse(tmpFile);
            Check(false, "[19] Missing FriendlyName throws PluginParseException");
        }
        catch (PluginParseException ex) when (ex.Message.Contains("FriendlyName"))
        {
            Check(true, "[19] Missing FriendlyName throws PluginParseException");
        }
        finally { TryDelete(tmpFile); }
    }

    /// <summary>Test [20]: Empty module name in Modules array.</summary>
    private static void TestEmptyModuleName()
    {
        Console.WriteLine("[Test 20] Empty module name");

        var json = """
        {
            "FileVersion": 1,
            "FriendlyName": "Bad Plugin",
            "Modules": [{"Name": "", "Type": "Runtime"}]
        }
        """;
        var tmpFile = WriteTempJson(json);
        try
        {
            PluginParser.Parse(tmpFile);
            Check(false, "[20] Empty module name throws PluginParseException");
        }
        catch (PluginParseException ex) when (ex.Message.Contains("Module at index 0"))
        {
            Check(true, "[20] Empty module name throws PluginParseException");
        }
        finally { TryDelete(tmpFile); }
    }

    /// <summary>Test [21]: Malformed JSON.</summary>
    private static void TestMalformedJson()
    {
        Console.WriteLine("[Test 21] Malformed JSON");

        var tmpFile = WriteTempJson("{not valid json!!!");
        try
        {
            PluginParser.Parse(tmpFile);
            Check(false, "[21] Malformed JSON throws PluginParseException");
        }
        catch (PluginParseException ex) when (ex.Message.Contains("JSON parse error"))
        {
            Check(true, "[21] Malformed JSON throws PluginParseException");
        }
        finally { TryDelete(tmpFile); }
    }

    /// <summary>Test [22]: Non-existent file.</summary>
    private static void TestFileNotFound()
    {
        Console.WriteLine("[Test 22] Non-existent file");

        try
        {
            PluginParser.Parse("NonExistent.eplugin");
            Check(false, "[22] Non-existent file throws FileNotFoundException");
        }
        catch (FileNotFoundException)
        {
            Check(true, "[22] Non-existent file throws FileNotFoundException");
        }
    }

    // ── Helpers ──

    private static string WriteTempJson(string content)
    {
        var path = Path.Combine(Path.GetTempPath(), $"test_{Guid.NewGuid():N}.eplugin");
        File.WriteAllText(path, content);
        return path;
    }

    private static void TryDelete(string path)
    {
        try { File.Delete(path); } catch { /* best effort */ }
    }

    private static void Check(bool cond, string name)
    {
        if (cond) { Console.WriteLine($"  [PASS] {name}"); ++_passed; }
        else      { Console.WriteLine($"  [FAIL] {name}"); ++_failed; }
    }
}
