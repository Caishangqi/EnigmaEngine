// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Tests;

using System.Text.Json.Nodes;
using BuildTool.Commands;
using BuildTool.Models;

/// <summary>
/// Unit tests for <see cref="CreateModuleCommand"/>.
/// </summary>
public static class CreateModuleCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== CreateModuleCommand Tests ===");
        Console.WriteLine();

        TestCreatesModuleFiles();
        TestUpdatesEproject();
        TestUpdatesTargetCs();
        TestRejectsInvalidName();
        TestRejectsDuplicateModule();
        TestRollbackOnFailure();
        TestDefaultModuleType();
        TestCustomModuleType();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestCreatesModuleFiles()
    {
        Console.WriteLine("[Test 1] Creates .Build.cs, Module.h, Module.cpp");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "NewModule");
            Assert(result.Success, $"Expected success: {result.Message}");

            var moduleDir = Path.Combine(ProjectRoot(root), "Source", "NewModule");
            Assert(Directory.Exists(moduleDir), "Module directory should exist");
            Assert(File.Exists(Path.Combine(moduleDir, "NewModule.Build.cs")), ".Build.cs missing");
            Assert(File.Exists(Path.Combine(moduleDir, "Public", "NewModuleModule.h")), "Module.h missing");
            Assert(File.Exists(Path.Combine(moduleDir, "Private", "NewModuleModule.cpp")), "Module.cpp missing");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestUpdatesEproject()
    {
        Console.WriteLine("[Test 2] Adds module entry to .eproject");

        var root = CreateTempProject();
        try
        {
            Execute(root, "Gameplay");

            var json = ParseEproject(root);
            var modules = json["Modules"]!.AsArray();
            bool found = false;
            foreach (var m in modules)
            {
                if (m?["Name"]?.GetValue<string>() == "Gameplay")
                { found = true; break; }
            }
            Assert(found, "Module 'Gameplay' should be in .eproject Modules array");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestUpdatesTargetCs()
    {
        Console.WriteLine("[Test 3] Adds ExtraModuleNames.Add() to .Target.cs");

        var root = CreateTempProject();
        try
        {
            Execute(root, "Networking");

            var targetPath = Path.Combine(ProjectRoot(root), "Source", "TestGame.Target.cs");
            var content = File.ReadAllText(targetPath);
            Assert(content.Contains("ExtraModuleNames.Add(\"Networking\");"),
                "Target should contain ExtraModuleNames.Add(\"Networking\")");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsInvalidName()
    {
        Console.WriteLine("[Test 4] Rejects invalid name '2Bad'");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "2Bad");
            Assert(!result.Success, "Should fail for name starting with digit");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRejectsDuplicateModule()
    {
        Console.WriteLine("[Test 5] Rejects duplicate module name");

        var root = CreateTempProject();
        try
        {
            // "TestGame" already exists as a game module
            var result = Execute(root, "TestGame");
            Assert(!result.Success, "Should fail for duplicate module name");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestRollbackOnFailure()
    {
        Console.WriteLine("[Test 6] Rollback cleans up on failure");

        var root = CreateTempProject();
        try
        {
            // Make .eproject read-only so EprojectModifier fails after templates are created
            var eprojectPath = Path.Combine(ProjectRoot(root), "TestGame.eproject");
            File.SetAttributes(eprojectPath, File.GetAttributes(eprojectPath) | FileAttributes.ReadOnly);

            var result = Execute(root, "RollbackMod");
            Assert(!result.Success, "Should fail due to read-only .eproject");

            // Module directory should be rolled back
            var moduleDir = Path.Combine(ProjectRoot(root), "Source", "RollbackMod");
            Assert(!Directory.Exists(moduleDir),
                "Module directory should not exist after rollback");

            // Restore attribute for cleanup
            File.SetAttributes(eprojectPath, File.GetAttributes(eprojectPath) & ~FileAttributes.ReadOnly);

            Console.WriteLine("  PASSED");
        }
        finally
        {
            // Ensure read-only is cleared before cleanup
            var ep = Path.Combine(ProjectRoot(root), "TestGame.eproject");
            if (File.Exists(ep))
            {
                try { File.SetAttributes(ep, FileAttributes.Normal); }
                catch { /* best effort */ }
            }
            CleanupDir(root);
        }
    }

    private static void TestDefaultModuleType()
    {
        Console.WriteLine("[Test 7] Default module type is 'Runtime'");

        var root = CreateTempProject();
        try
        {
            Execute(root, "DefaultMod");

            var json = ParseEproject(root);
            var modules = json["Modules"]!.AsArray();
            string? type = null;
            foreach (var m in modules)
            {
                if (m?["Name"]?.GetValue<string>() == "DefaultMod")
                { type = m?["Type"]?.GetValue<string>(); break; }
            }
            Assert(type == "Runtime", $"Expected 'Runtime', got '{type}'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    private static void TestCustomModuleType()
    {
        Console.WriteLine("[Test 8] Custom module type 'Editor'");

        var root = CreateTempProject();
        try
        {
            var result = Execute(root, "EditorMod", moduleType: "Editor");
            Assert(result.Success, $"Expected success: {result.Message}");

            var json = ParseEproject(root);
            var modules = json["Modules"]!.AsArray();
            string? type = null;
            foreach (var m in modules)
            {
                if (m?["Name"]?.GetValue<string>() == "EditorMod")
                { type = m?["Type"]?.GetValue<string>(); break; }
            }
            Assert(type == "Editor", $"Expected 'Editor', got '{type}'");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    // --- Helpers ---

    private static string ProjectRoot(string root) =>
        Path.Combine(root, "Games", "TestGame");

    private static BuildResult Execute(string root, string moduleName, string? moduleType = null)
    {
        var extra = new Dictionary<string, string> { ["name"] = moduleName };
        if (moduleType is not null)
            extra["type"] = moduleType;

        var options = new BuildOptions
        {
            ProjectPath = ProjectRoot(root),
            ExtraArguments = extra,
        };
        return new CreateModuleCommand().Execute(options);
    }

    private static JsonObject ParseEproject(string root)
    {
        var path = Path.Combine(ProjectRoot(root), "TestGame.eproject");
        return JsonNode.Parse(File.ReadAllText(path))!.AsObject();
    }

    /// <summary>
    /// Creates a minimal but valid project structure for CreateModuleCommand tests.
    /// </summary>
    private static string CreateTempProject()
    {
        var root = Path.Combine(Path.GetTempPath(), $"enigma_test_{Guid.NewGuid():N}");

        // Engine/Source/Runtime/ (required for FindEngineRoot)
        Directory.CreateDirectory(Path.Combine(root, "Engine", "Source", "Runtime"));

        // Module templates
        var tplDir = Path.Combine(root, "Engine", "Templates", "Module");
        Directory.CreateDirectory(Path.Combine(tplDir, "Public"));
        Directory.CreateDirectory(Path.Combine(tplDir, "Private"));

        File.WriteAllText(
            Path.Combine(tplDir, "MODULE_NAME.Build.cs.template"),
            """
            using EnigmaEngine;
            public class MODULE_NAME : ModuleRules
            {
                public MODULE_NAME(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicIncludePaths.Add("Public");
                    PrivateIncludePaths.Add("Private");
                }
            }
            """);
        File.WriteAllText(
            Path.Combine(tplDir, "Public", "MODULE_NAMEModule.h.template"),
            """
            #pragma once
            class MODULE_NAME_API MODULE_NAMEModule {};
            """);
        File.WriteAllText(
            Path.Combine(tplDir, "Private", "MODULE_NAMEModule.cpp.template"),
            """
            #include "MODULE_NAMEModule.h"
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
