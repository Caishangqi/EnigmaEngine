// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Tests;

using System.Text.Json.Nodes;
using BuildTool.Commands;
using BuildTool.Models;

/// <summary>
/// Unit tests for <see cref="RemovePluginCommand"/>.
/// </summary>
public static class RemovePluginCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== RemovePluginCommand Tests ===");
        Console.WriteLine();

        TestRemovesPluginDirectory();
        TestUpdatesEproject();
        TestDoesNotModifyTargetCs();
        TestBlocksOnDependents();
        TestForceOverridesDependencyCheck();
        TestRejectsNonexistentPlugin();
        TestRegeneratesProjectFiles();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestRemovesPluginDirectory()
    {
        Console.WriteLine("[Test 1] Removes plugin directory");

        var root = CreateTempProject();
        try
        {
            var pluginDir = Path.Combine(ProjectRoot(root), "Plugins", "TestPlugin");
            Assert(Directory.Exists(pluginDir), "TestPlugin dir should exist before removal");

            var result = Execute(root, "TestPlugin");
            Assert(result.Success, $"Expected success: {result.Message}");
            Assert(!Directory.Exists(pluginDir), "TestPlugin dir should be deleted");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestUpdatesEproject()
    {
        Console.WriteLine("[Test 2] Removes plugin entry from .eproject");

        var root = CreateTempProject();
        try
        {
            Execute(root, "TestPlugin");

            var json = ParseEproject(root);
            var plugins = json["Plugins"]!.AsArray();
            bool found = false;
            foreach (var p in plugins)
            {
                if (p?["Name"]?.GetValue<string>() == "TestPlugin")
                { found = true; break; }
            }
            Assert(!found, "TestPlugin should NOT be in .eproject after removal");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestDoesNotModifyTargetCs()
    {
        Console.WriteLine("[Test 3] Does not modify .Target.cs");

        var root = CreateTempProject();
        try
        {
            var targetPath = Path.Combine(ProjectRoot(root), "Source", "TestGame.Target.cs");
            var before = File.ReadAllText(targetPath);

            Execute(root, "TestPlugin");

            var after = File.ReadAllText(targetPath);
            Assert(before == after, ".Target.cs should not be modified by remove-plugin");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestBlocksOnDependents()
    {
        Console.WriteLine("[Test 4] Blocks removal when game modules depend on plugin module");

        var root = CreateTempProjectWithDeps();
        try
        {
            // TestGame depends on DepPlugin's module, so removal should be blocked
            var result = Execute(root, "DepPlugin");
            Assert(!result.Success, "Should fail due to dependents");
            Assert(result.Message.Contains("TestGame"), "Error should mention dependent 'TestGame'");

            // Plugin directory should still exist
            var pluginDir = Path.Combine(ProjectRoot(root), "Plugins", "DepPlugin");
            Assert(Directory.Exists(pluginDir), "DepPlugin dir should still exist");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestForceOverridesDependencyCheck()
    {
        Console.WriteLine("[Test 5] --force overrides dependency check");

        var root = CreateTempProjectWithDeps();
        try
        {
            var result = Execute(root, "DepPlugin", force: true);
            Assert(result.Success, $"Expected success with --force: {result.Message}");

            var pluginDir = Path.Combine(ProjectRoot(root), "Plugins", "DepPlugin");
            Assert(!Directory.Exists(pluginDir), "DepPlugin dir should be deleted with --force");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsNonexistentPlugin()
    {
        Console.WriteLine("[Test 6] Rejects nonexistent plugin");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "GhostPlugin");
            Assert(!result.Success, "Should fail for nonexistent plugin");
            Assert(result.Message.Contains("not found"), "Error should say 'not found'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRegeneratesProjectFiles()
    {
        Console.WriteLine("[Test 7] Succeeds end-to-end including regeneration attempt");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "TestPlugin");
            Assert(result.Success, $"Expected success: {result.Message}");
            Assert(result.Message.Contains("removed successfully"),
                "Success message should confirm removal");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    // --- Helpers ---

    private static string ProjectRoot(string root) =>
        Path.Combine(root, "Games", "TestGame");

    private static BuildResult Execute(string root, string pluginName, bool force = false)
    {
        var extra = new Dictionary<string, string> { ["name"] = pluginName };
        if (force)
            extra["force"] = "true";

        var options = new BuildOptions
        {
            ProjectPath = ProjectRoot(root),
            ExtraArguments = extra,
        };
        return new RemovePluginCommand().Execute(options);
    }

    private static JsonObject ParseEproject(string root)
    {
        var path = Path.Combine(ProjectRoot(root), "TestGame.eproject");
        return JsonNode.Parse(File.ReadAllText(path))!.AsObject();
    }

    /// <summary>
    /// Creates a temp project with TestGame module + TestPlugin (enabled plugin with one module).
    /// </summary>
    private static string CreateTempProject()
    {
        var root = Path.Combine(Path.GetTempPath(), $"enigma_test_{Guid.NewGuid():N}");

        // Engine/Source/Runtime/ (required for FindEngineRoot)
        Directory.CreateDirectory(Path.Combine(root, "Engine", "Source", "Runtime"));

        // Project structure
        var proj = Path.Combine(root, "Games", "TestGame");
        var src = Path.Combine(proj, "Source");

        // TestGame module
        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Public"));
        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Private"));
        File.WriteAllText(Path.Combine(src, "TestGame", "TestGame.Build.cs"), """
            using EnigmaEngine;
            public class TestGame : ModuleRules
            {
                public TestGame(ReadOnlyTargetRules Target) : base(Target) { }
            }
            """);

        // TestPlugin - pre-created plugin with .eplugin and source module
        var pluginDir = Path.Combine(proj, "Plugins", "TestPlugin");
        var pluginSrc = Path.Combine(pluginDir, "Source", "TestPlugin");
        Directory.CreateDirectory(Path.Combine(pluginSrc, "Public"));
        Directory.CreateDirectory(Path.Combine(pluginSrc, "Private"));

        File.WriteAllText(Path.Combine(pluginDir, "TestPlugin.eplugin"), """
            {
                "FileVersion": 1,
                "FriendlyName": "TestPlugin",
                "Description": "Test plugin",
                "Category": "Gameplay",
                "Modules": [
                    { "Name": "TestPlugin", "Type": "Runtime", "LoadingPhase": "PostEngineInit" }
                ]
            }
            """);
        File.WriteAllText(Path.Combine(pluginSrc, "TestPlugin.Build.cs"), """
            using EnigmaEngine;
            public class TestPlugin : ModuleRules
            {
                public TestPlugin(ReadOnlyTargetRules Target) : base(Target) { }
            }
            """);
        File.WriteAllText(Path.Combine(pluginSrc, "Public", "TestPluginModule.h"), """
            #pragma once
            class TestPluginModule {};
            """);
        File.WriteAllText(Path.Combine(pluginSrc, "Private", "TestPluginModule.cpp"), """
            #include "TestPluginModule.h"
            """);

        // .eproject (with TestPlugin in Plugins)
        File.WriteAllText(Path.Combine(proj, "TestGame.eproject"), """
            {
                "FileVersion": 1,
                "EngineAssociation": "0.1.0",
                "Modules": [
                    { "Name": "TestGame", "Type": "Runtime", "LoadingPhase": "Default" }
                ],
                "Plugins": [
                    { "Name": "TestPlugin", "Enabled": true }
                ]
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

        return root;
    }

    /// <summary>
    /// Creates a temp project where TestGame depends on DepPlugin's module.
    /// </summary>
    private static string CreateTempProjectWithDeps()
    {
        var root = Path.Combine(Path.GetTempPath(), $"enigma_test_{Guid.NewGuid():N}");

        // Engine/Source/Runtime/
        Directory.CreateDirectory(Path.Combine(root, "Engine", "Source", "Runtime"));

        var proj = Path.Combine(root, "Games", "TestGame");
        var src = Path.Combine(proj, "Source");

        // TestGame module - depends on DepPlugin module
        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Public"));
        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Private"));
        File.WriteAllText(Path.Combine(src, "TestGame", "TestGame.Build.cs"), """
            using EnigmaEngine;
            public class TestGame : ModuleRules
            {
                public TestGame(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicDependencyModuleNames.Add("DepPlugin");
                }
            }
            """);

        // DepPlugin - plugin whose module is depended upon
        var pluginDir = Path.Combine(proj, "Plugins", "DepPlugin");
        var pluginSrc = Path.Combine(pluginDir, "Source", "DepPlugin");
        Directory.CreateDirectory(Path.Combine(pluginSrc, "Public"));
        Directory.CreateDirectory(Path.Combine(pluginSrc, "Private"));

        File.WriteAllText(Path.Combine(pluginDir, "DepPlugin.eplugin"), """
            {
                "FileVersion": 1,
                "FriendlyName": "DepPlugin",
                "Description": "Dependency plugin",
                "Category": "Gameplay",
                "Modules": [
                    { "Name": "DepPlugin", "Type": "Runtime", "LoadingPhase": "PostEngineInit" }
                ]
            }
            """);
        File.WriteAllText(Path.Combine(pluginSrc, "DepPlugin.Build.cs"), """
            using EnigmaEngine;
            public class DepPlugin : ModuleRules
            {
                public DepPlugin(ReadOnlyTargetRules Target) : base(Target) { }
            }
            """);

        // .eproject with DepPlugin enabled
        File.WriteAllText(Path.Combine(proj, "TestGame.eproject"), """
            {
                "FileVersion": 1,
                "EngineAssociation": "0.1.0",
                "Modules": [
                    { "Name": "TestGame", "Type": "Runtime", "LoadingPhase": "Default" }
                ],
                "Plugins": [
                    { "Name": "DepPlugin", "Enabled": true }
                ]
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
