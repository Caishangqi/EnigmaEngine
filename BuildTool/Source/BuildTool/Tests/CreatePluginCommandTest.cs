// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Tests;

using System.Text.Json.Nodes;
using BuildTool.Commands;
using BuildTool.Models;

/// <summary>
/// Unit tests for <see cref="CreatePluginCommand"/>.
/// </summary>
public static class CreatePluginCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== CreatePluginCommand Tests ===");
        Console.WriteLine();

        TestCreatesPluginStructure();
        TestCreatesEpluginDescriptor();
        TestUpdatesEproject();
        TestDoesNotModifyTargetCs();
        TestRejectsInvalidName();
        TestRejectsDuplicatePlugin();
        TestDefaultCategory();
        TestCustomCategory();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestCreatesPluginStructure()
    {
        Console.WriteLine("[Test 1] Creates full plugin directory structure");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "MyPlugin");
            Assert(result.Success, $"Expected success: {result.Message}");

            var pluginDir = Path.Combine(ProjectRoot(root), "Plugins", "MyPlugin");
            Assert(Directory.Exists(pluginDir), "Plugin directory should exist");
            Assert(File.Exists(Path.Combine(pluginDir, "MyPlugin.eplugin")), ".eplugin missing");
            Assert(File.Exists(Path.Combine(pluginDir, "Source", "MyPlugin", "MyPlugin.Build.cs")), ".Build.cs missing");
            Assert(File.Exists(Path.Combine(pluginDir, "Source", "MyPlugin", "Public", "MyPluginModule.h")), "Module.h missing");
            Assert(File.Exists(Path.Combine(pluginDir, "Source", "MyPlugin", "Private", "MyPluginModule.cpp")), "Module.cpp missing");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestCreatesEpluginDescriptor()
    {
        Console.WriteLine("[Test 2] .eplugin has valid JSON with correct fields");

        var root = CreateTempProject();
        try
        {
            Execute(root, "CoolPlugin", category: "Rendering");

            var epluginPath = Path.Combine(ProjectRoot(root), "Plugins", "CoolPlugin", "CoolPlugin.eplugin");
            var json = JsonNode.Parse(File.ReadAllText(epluginPath))!.AsObject();

            Assert(json["FriendlyName"]!.GetValue<string>() == "CoolPlugin",
                $"FriendlyName mismatch: {json["FriendlyName"]}");
            Assert(json["Category"]!.GetValue<string>() == "Rendering",
                $"Category mismatch: {json["Category"]}");

            var modules = json["Modules"]!.AsArray();
            Assert(modules.Count >= 1, "Should have at least one module");
            Assert(modules[0]!["Name"]!.GetValue<string>() == "CoolPlugin",
                "Module name should match plugin name");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestUpdatesEproject()
    {
        Console.WriteLine("[Test 3] Adds plugin entry to .eproject");

        var root = CreateTempProject();
        try
        {
            Execute(root, "NetPlugin");

            var json = ParseEproject(root);
            var plugins = json["Plugins"]!.AsArray();
            bool found = false;
            foreach (var p in plugins)
            {
                if (p?["Name"]?.GetValue<string>() == "NetPlugin")
                { found = true; break; }
            }
            Assert(found, "Plugin 'NetPlugin' should be in .eproject Plugins array");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestDoesNotModifyTargetCs()
    {
        Console.WriteLine("[Test 4] Does not modify .Target.cs");

        var root = CreateTempProject();
        try
        {
            var targetPath = Path.Combine(ProjectRoot(root), "Source", "TestGame.Target.cs");
            var before = File.ReadAllText(targetPath);

            Execute(root, "SomePlugin");

            var after = File.ReadAllText(targetPath);
            Assert(before == after, ".Target.cs should not be modified by create-plugin");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsInvalidName()
    {
        Console.WriteLine("[Test 5] Rejects invalid name '2Bad'");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "2Bad");
            Assert(!result.Success, "Should fail for name starting with digit");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsDuplicatePlugin()
    {
        Console.WriteLine("[Test 6] Rejects duplicate plugin name");

        var root = CreateTempProject();
        try
        {
            var first = Execute(root, "DupPlugin");
            Assert(first.Success, $"First create should succeed: {first.Message}");

            var second = Execute(root, "DupPlugin");
            Assert(!second.Success, "Second create should fail for duplicate");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestDefaultCategory()
    {
        Console.WriteLine("[Test 7] Default category is 'Gameplay'");

        var root = CreateTempProject();
        try
        {
            Execute(root, "DefPlugin");

            var epluginPath = Path.Combine(ProjectRoot(root), "Plugins", "DefPlugin", "DefPlugin.eplugin");
            var json = JsonNode.Parse(File.ReadAllText(epluginPath))!.AsObject();
            var cat = json["Category"]!.GetValue<string>();
            Assert(cat == "Gameplay", $"Expected 'Gameplay', got '{cat}'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestCustomCategory()
    {
        Console.WriteLine("[Test 8] Custom category 'AI'");

        var root = CreateTempProject();
        try
        {
            Execute(root, "AiPlugin", category: "AI");

            var epluginPath = Path.Combine(ProjectRoot(root), "Plugins", "AiPlugin", "AiPlugin.eplugin");
            var json = JsonNode.Parse(File.ReadAllText(epluginPath))!.AsObject();
            var cat = json["Category"]!.GetValue<string>();
            Assert(cat == "AI", $"Expected 'AI', got '{cat}'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    // --- Helpers ---

    private static string ProjectRoot(string root) =>
        Path.Combine(root, "Games", "TestGame");

    private static BuildResult Execute(string root, string pluginName, string? category = null)
    {
        var extra = new Dictionary<string, string> { ["name"] = pluginName };
        if (category is not null)
            extra["category"] = category;

        var options = new BuildOptions
        {
            ProjectPath = ProjectRoot(root),
            ExtraArguments = extra,
        };
        return new CreatePluginCommand().Execute(options);
    }

    private static JsonObject ParseEproject(string root)
    {
        var path = Path.Combine(ProjectRoot(root), "TestGame.eproject");
        return JsonNode.Parse(File.ReadAllText(path))!.AsObject();
    }

    /// <summary>
    /// Creates a minimal valid project structure for CreatePluginCommand tests.
    /// Includes Engine/Source/Runtime, Plugin templates, .eproject, .Target.cs, game module.
    /// </summary>
    private static string CreateTempProject()
    {
        var root = Path.Combine(Path.GetTempPath(), $"enigma_test_{Guid.NewGuid():N}");

        // Engine/Source/Runtime/ (required for FindEngineRoot)
        Directory.CreateDirectory(Path.Combine(root, "Engine", "Source", "Runtime"));

        // Plugin templates
        var tplDir = Path.Combine(root, "Engine", "Templates", "Plugin");
        var tplSrcDir = Path.Combine(tplDir, "Source", "PLUGIN_NAME");
        Directory.CreateDirectory(Path.Combine(tplSrcDir, "Public"));
        Directory.CreateDirectory(Path.Combine(tplSrcDir, "Private"));

        File.WriteAllText(
            Path.Combine(tplDir, "PLUGIN_NAME.eplugin.template"),
            """
            {
                "FileVersion": 1,
                "FriendlyName": "PLUGIN_NAME",
                "Description": "PLUGIN_NAME plugin for EnigmaEngine",
                "Category": "PLUGIN_CATEGORY",
                "Modules": [
                    {
                        "Name": "PLUGIN_NAME",
                        "Type": "Runtime",
                        "LoadingPhase": "PostEngineInit"
                    }
                ]
            }
            """);
        File.WriteAllText(
            Path.Combine(tplSrcDir, "PLUGIN_NAME.Build.cs.template"),
            """
            using EnigmaEngine;
            public class PLUGIN_NAME : ModuleRules
            {
                public PLUGIN_NAME(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicIncludePaths.Add("Public");
                    PrivateIncludePaths.Add("Private");
                }
            }
            """);
        File.WriteAllText(
            Path.Combine(tplSrcDir, "Public", "PLUGIN_NAMEModule.h.template"),
            """
            #pragma once
            class PLUGIN_NAME_API PLUGIN_NAMEModule {};
            """);
        File.WriteAllText(
            Path.Combine(tplSrcDir, "Private", "PLUGIN_NAMEModule.cpp.template"),
            """
            #include "PLUGIN_NAMEModule.h"
            """);

        // Project structure
        var proj = Path.Combine(root, "Games", "TestGame");
        var src = Path.Combine(proj, "Source");
        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Public"));
        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Private"));

        // .eproject
        File.WriteAllText(Path.Combine(proj, "TestGame.eproject"), """
            {
                "FileVersion": 1,
                "EngineAssociation": "0.1.0",
                "Modules": [
                    { "Name": "TestGame", "Type": "Runtime", "LoadingPhase": "Default" }
                ],
                "Plugins": []
            }
            """);

        // .Target.cs
        File.WriteAllText(Path.Combine(src, "TestGame.Target.cs"), """
            using EnigmaBuildTool;

            public class TestGameTarget : TargetRules
            {
                public TestGameTarget(TargetInfo Target) : base(Target)
                {
                    Type = TargetType.Game;
                    ExtraModuleNames.Add("TestGame");
                }
            }
            """);

        // Game module .Build.cs
        File.WriteAllText(Path.Combine(src, "TestGame", "TestGame.Build.cs"), """
            using EnigmaEngine;

            public class TestGame : ModuleRules
            {
                public TestGame(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicIncludePaths.Add("Public");
                    PrivateIncludePaths.Add("Private");
                }
            }
            """);

        return root;
    }

    private static void CleanupDir(string path)
    {
        try
        {
            if (Directory.Exists(path))
                Directory.Delete(path, recursive: true);
        }
        catch { /* Best effort */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
