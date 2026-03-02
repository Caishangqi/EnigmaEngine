// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Tests;

using System.Text.Json.Nodes;
using BuildTool.Commands;
using BuildTool.Models;

/// <summary>
/// Unit tests for <see cref="RemoveModuleCommand"/>.
/// </summary>
public static class RemoveModuleCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== RemoveModuleCommand Tests ===");
        Console.WriteLine();

        TestRemovesModuleDirectory();
        TestUpdatesEproject();
        TestUpdatesTargetCs();
        TestBlocksOnDependents();
        TestForceOverridesDependencyCheck();
        TestRejectsNonexistentModule();
        TestRejectsPrimaryModule();
        TestRejectsEngineModule();
        TestRegeneratesProjectFiles();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestRemovesModuleDirectory()
    {
        Console.WriteLine("[Test 1] Removes module source directory");

        var root = CreateTempProject();
        try
        {
            var moduleDir = Path.Combine(ProjectRoot(root), "Source", "ExtraMod");
            Assert(Directory.Exists(moduleDir), "ExtraMod dir should exist before removal");

            var result = Execute(root, "ExtraMod");
            Assert(result.Success, $"Expected success: {result.Message}");
            Assert(!Directory.Exists(moduleDir), "ExtraMod dir should be deleted");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestUpdatesEproject()
    {
        Console.WriteLine("[Test 2] Removes module entry from .eproject");

        var root = CreateTempProject();
        try
        {
            Execute(root, "ExtraMod");

            var json = ParseEproject(root);
            var modules = json["Modules"]!.AsArray();
            bool found = false;
            foreach (var m in modules)
            {
                if (m?["Name"]?.GetValue<string>() == "ExtraMod")
                { found = true; break; }
            }
            Assert(!found, "ExtraMod should NOT be in .eproject after removal");

            // Primary module should still be there
            bool primaryFound = false;
            foreach (var m in modules)
            {
                if (m?["Name"]?.GetValue<string>() == "TestGame")
                { primaryFound = true; break; }
            }
            Assert(primaryFound, "TestGame should still be in .eproject");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestUpdatesTargetCs()
    {
        Console.WriteLine("[Test 3] Removes ExtraModuleNames.Add() from .Target.cs");

        var root = CreateTempProject();
        try
        {
            Execute(root, "ExtraMod");

            var targetPath = Path.Combine(ProjectRoot(root), "Source", "TestGame.Target.cs");
            var content = File.ReadAllText(targetPath);
            Assert(!content.Contains("ExtraModuleNames.Add(\"ExtraMod\")"),
                "ExtraMod Add() line should be removed");
            Assert(content.Contains("ExtraModuleNames.Add(\"TestGame\")"),
                "TestGame Add() line should remain");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestBlocksOnDependents()
    {
        Console.WriteLine("[Test 4] Blocks removal when other modules depend on target");

        var root = CreateTempProjectWithDeps();
        try
        {
            // DepB depends on DepA, so removing DepA should be blocked
            var result = Execute(root, "DepA");
            Assert(!result.Success, "Should fail due to dependents");
            Assert(result.Message.Contains("DepB"), "Error should mention dependent 'DepB'");

            // DepA directory should still exist
            var depADir = Path.Combine(ProjectRoot(root), "Source", "DepA");
            Assert(Directory.Exists(depADir), "DepA dir should still exist");

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
            var result = Execute(root, "DepA", force: true);
            Assert(result.Success, $"Expected success with --force: {result.Message}");

            var depADir = Path.Combine(ProjectRoot(root), "Source", "DepA");
            Assert(!Directory.Exists(depADir), "DepA dir should be deleted with --force");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsNonexistentModule()
    {
        Console.WriteLine("[Test 6] Rejects nonexistent module");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "GhostModule");
            Assert(!result.Success, "Should fail for nonexistent module");
            Assert(result.Message.Contains("not found"), "Error should say 'not found'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsPrimaryModule()
    {
        Console.WriteLine("[Test 7] Rejects primary module removal");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "TestGame");
            Assert(!result.Success, "Should fail for primary module");
            Assert(result.Message.Contains("primary") || result.Message.Contains("main"),
                "Error should mention primary/main module");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsEngineModule()
    {
        Console.WriteLine("[Test 8] Rejects engine module 'Core'");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "Core");
            Assert(!result.Success, "Should fail for engine module");
            Assert(result.Message.Contains("engine module"),
                "Error should mention engine module");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRegeneratesProjectFiles()
    {
        Console.WriteLine("[Test 9] Succeeds end-to-end including regeneration attempt");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "ExtraMod");
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

    private static BuildResult Execute(string root, string moduleName, bool force = false)
    {
        var extra = new Dictionary<string, string> { ["name"] = moduleName };
        if (force)
            extra["force"] = "true";

        var options = new BuildOptions
        {
            ProjectPath = ProjectRoot(root),
            ExtraArguments = extra,
        };
        return new RemoveModuleCommand().Execute(options);
    }

    private static JsonObject ParseEproject(string root)
    {
        var path = Path.Combine(ProjectRoot(root), "TestGame.eproject");
        return JsonNode.Parse(File.ReadAllText(path))!.AsObject();
    }

    /// <summary>
    /// Creates a temp project with TestGame (primary) + ExtraMod + engine Core module.
    /// </summary>
    private static string CreateTempProject()
    {
        var root = Path.Combine(Path.GetTempPath(), $"enigma_test_{Guid.NewGuid():N}");

        // Engine/Source/Runtime/Core/ (engine module)
        var coreDir = Path.Combine(root, "Engine", "Source", "Runtime", "Core");
        Directory.CreateDirectory(coreDir);
        File.WriteAllText(Path.Combine(coreDir, "Core.Build.cs"), """
            using EnigmaEngine;
            public class Core : ModuleRules
            {
                public Core(ReadOnlyTargetRules Target) : base(Target) { }
            }
            """);

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
                public TestGame(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicDependencyModuleNames.Add("Core");
                }
            }
            """);

        // ExtraMod module
        Directory.CreateDirectory(Path.Combine(src, "ExtraMod", "Public"));
        Directory.CreateDirectory(Path.Combine(src, "ExtraMod", "Private"));
        File.WriteAllText(Path.Combine(src, "ExtraMod", "ExtraMod.Build.cs"), """
            using EnigmaEngine;
            public class ExtraMod : ModuleRules
            {
                public ExtraMod(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicDependencyModuleNames.Add("Core");
                }
            }
            """);

        // .eproject
        File.WriteAllText(Path.Combine(proj, "TestGame.eproject"), """
            {
                "FileVersion": 1,
                "EngineAssociation": "0.1.0",
                "Modules": [
                    { "Name": "TestGame", "Type": "Runtime", "LoadingPhase": "Default" },
                    { "Name": "ExtraMod", "Type": "Runtime", "LoadingPhase": "Default" }
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
                    ExtraModuleNames.Add("ExtraMod");
                }
            }
            """);

        return root;
    }

    /// <summary>
    /// Creates a temp project with dependency chain: DepB depends on DepA.
    /// </summary>
    private static string CreateTempProjectWithDeps()
    {
        var root = CreateTempProject();
        var src = Path.Combine(ProjectRoot(root), "Source");

        // DepA module (no special dependencies)
        Directory.CreateDirectory(Path.Combine(src, "DepA", "Public"));
        Directory.CreateDirectory(Path.Combine(src, "DepA", "Private"));
        File.WriteAllText(Path.Combine(src, "DepA", "DepA.Build.cs"), """
            using EnigmaEngine;
            public class DepA : ModuleRules
            {
                public DepA(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicDependencyModuleNames.Add("Core");
                }
            }
            """);

        // DepB module (depends on DepA)
        Directory.CreateDirectory(Path.Combine(src, "DepB", "Public"));
        Directory.CreateDirectory(Path.Combine(src, "DepB", "Private"));
        File.WriteAllText(Path.Combine(src, "DepB", "DepB.Build.cs"), """
            using EnigmaEngine;
            public class DepB : ModuleRules
            {
                public DepB(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicDependencyModuleNames.Add("DepA");
                }
            }
            """);

        // Update .eproject to include DepA and DepB
        var eprojectPath = Path.Combine(ProjectRoot(root), "TestGame.eproject");
        File.WriteAllText(eprojectPath, """
            {
                "FileVersion": 1,
                "EngineAssociation": "0.1.0",
                "Modules": [
                    { "Name": "TestGame", "Type": "Runtime", "LoadingPhase": "Default" },
                    { "Name": "ExtraMod", "Type": "Runtime", "LoadingPhase": "Default" },
                    { "Name": "DepA", "Type": "Runtime", "LoadingPhase": "Default" },
                    { "Name": "DepB", "Type": "Runtime", "LoadingPhase": "Default" }
                ],
                "Plugins": []
            }
            """);

        // Update .Target.cs to include DepA and DepB
        File.WriteAllText(Path.Combine(src, "TestGame.Target.cs"), """
            using EnigmaBuildTool;

            public class TestGameTarget : TargetRules
            {
                public TestGameTarget(TargetInfo Target) : base(Target)
                {
                    Type = TargetType.Game;
                    ExtraModuleNames.Add("TestGame");
                    ExtraModuleNames.Add("ExtraMod");
                    ExtraModuleNames.Add("DepA");
                    ExtraModuleNames.Add("DepB");
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
