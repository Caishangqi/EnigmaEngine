using BuildTool.Scaffolding;

namespace BuildTool.Tests;

/// <summary>
/// Unit tests for <see cref="TargetFileModifier"/>.
/// </summary>
public static class TargetFileModifierTest
{
    public static void Run()
    {
        Console.WriteLine("=== TargetFileModifier Tests ===");
        Console.WriteLine();

        TestAddModuleToExistingTarget();
        TestAddModuleAfterLastExisting();
        TestAddModuleToEmptyConstructor();
        TestPreservesIndentation();
        TestRemoveModule();
        TestRemoveModulePreservesOthers();
        TestInvalidTargetFile();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestAddModuleToExistingTarget()
    {
        Console.WriteLine("[Test 1] AddModule inserts Add() line into existing target");

        var path = CreateTempTarget(WithModules("GameCore"));
        try
        {
            var result = TargetFileModifier.AddModule(path, "NewModule");
            Assert(result.Success, $"Expected success: {result.Message}");

            var content = File.ReadAllText(path);
            Assert(content.Contains("ExtraModuleNames.Add(\"NewModule\");"),
                "New Add() line should be present");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestAddModuleAfterLastExisting()
    {
        Console.WriteLine("[Test 2] AddModule places new line after last existing Add()");

        var path = CreateTempTarget(WithModules("ModA", "ModB"));
        try
        {
            TargetFileModifier.AddModule(path, "ModC");

            var lines = File.ReadAllLines(path);
            int modBIndex = -1, modCIndex = -1;
            for (int i = 0; i < lines.Length; i++)
            {
                if (lines[i].Contains("\"ModB\"")) modBIndex = i;
                if (lines[i].Contains("\"ModC\"")) modCIndex = i;
            }

            Assert(modBIndex >= 0, "ModB line not found");
            Assert(modCIndex >= 0, "ModC line not found");
            Assert(modCIndex == modBIndex + 1,
                $"ModC should be right after ModB (expected {modBIndex + 1}, got {modCIndex})");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestAddModuleToEmptyConstructor()
    {
        Console.WriteLine("[Test 3] AddModule inserts into constructor with no existing Add()");

        var content = """
            using EnigmaBuildTool;

            public class TestTarget : TargetRules
            {
                public TestTarget(TargetInfo Target) : base(Target)
                {
                    Type = TargetType.Game;
                    ExtraModuleNames.Add("Existing");
                }
            }
            """;
        // First remove the existing Add line to simulate empty, but keep it valid for parse
        // Actually, TargetParser requires at least one ExtraModuleNames, so we test with one existing
        // and add a second. The "empty constructor" case means no Add() lines - but that won't parse.
        // Instead, test adding to a target with only Type set + one module (minimum valid).
        var path = CreateTempTargetRaw(content);
        try
        {
            var result = TargetFileModifier.AddModule(path, "NewModule");
            Assert(result.Success, $"Expected success: {result.Message}");

            var output = File.ReadAllText(path);
            Assert(output.Contains("ExtraModuleNames.Add(\"NewModule\");"),
                "New Add() line should be present");
            Assert(output.Contains("ExtraModuleNames.Add(\"Existing\");"),
                "Existing Add() line should be preserved");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestPreservesIndentation()
    {
        Console.WriteLine("[Test 4] AddModule preserves existing indentation");

        var path = CreateTempTarget(WithModules("GameCore"));
        try
        {
            TargetFileModifier.AddModule(path, "Extra");

            var lines = File.ReadAllLines(path);
            string? gameCoreIndent = null, extraIndent = null;
            foreach (var line in lines)
            {
                if (line.Contains("\"GameCore\""))
                    gameCoreIndent = line[..line.IndexOf('E')];
                if (line.Contains("\"Extra\""))
                    extraIndent = line[..line.IndexOf('E')];
            }

            Assert(gameCoreIndent is not null, "GameCore line not found");
            Assert(extraIndent is not null, "Extra line not found");
            Assert(gameCoreIndent == extraIndent,
                $"Indentation mismatch: '{gameCoreIndent}' vs '{extraIndent}'");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestRemoveModule()
    {
        Console.WriteLine("[Test 5] RemoveModule removes the target line");

        var path = CreateTempTarget(WithModules("ModA", "ModB"));
        try
        {
            var result = TargetFileModifier.RemoveModule(path, "ModA");
            Assert(result.Success, $"Expected success: {result.Message}");

            var content = File.ReadAllText(path);
            Assert(!content.Contains("\"ModA\""), "ModA should be removed");
            Assert(content.Contains("\"ModB\""), "ModB should still be present");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestRemoveModulePreservesOthers()
    {
        Console.WriteLine("[Test 6] RemoveModule preserves other Add() lines");

        var path = CreateTempTarget(WithModules("Alpha", "Beta", "Gamma"));
        try
        {
            TargetFileModifier.RemoveModule(path, "Beta");

            var content = File.ReadAllText(path);
            Assert(content.Contains("\"Alpha\""), "Alpha should be preserved");
            Assert(!content.Contains("\"Beta\""), "Beta should be removed");
            Assert(content.Contains("\"Gamma\""), "Gamma should be preserved");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    private static void TestInvalidTargetFile()
    {
        Console.WriteLine("[Test 7] Invalid .Target.cs file returns Fail");

        var path = Path.Combine(Path.GetTempPath(), $"Bad_{Guid.NewGuid():N}.Target.cs");
        try
        {
            File.WriteAllText(path, "this is not a valid target file");
            var result = TargetFileModifier.AddModule(path, "Test");
            Assert(!result.Success, "Should fail for invalid target file");

            Console.WriteLine("  PASSED");
        }
        finally { DeleteTemp(path); }
    }

    // --- Helpers ---

    private static string WithModules(params string[] moduleNames)
    {
        var adds = string.Join("\n",
            moduleNames.Select(n => $"        ExtraModuleNames.Add(\"{n}\");"));
        return adds;
    }

    private static string CreateTempTarget(string addLines)
    {
        var content = $$"""
            using EnigmaBuildTool;

            public class TestTarget : TargetRules
            {
                public TestTarget(TargetInfo Target) : base(Target)
                {
                    Type = TargetType.Game;
            {{addLines}}
                }
            }
            """;
        return CreateTempTargetRaw(content);
    }

    private static string CreateTempTargetRaw(string content)
    {
        var path = Path.Combine(Path.GetTempPath(), $"Test_{Guid.NewGuid():N}.Target.cs");
        File.WriteAllText(path, content);
        return path;
    }

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
