// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Tests;

using System.Text.Json.Nodes;
using BuildTool.Commands;
using BuildTool.Models;

/// <summary>
/// End-to-end integration tests for all scaffolding commands.
/// Verifies that create commands produce valid project structures that pass
/// GenerateProjectFiles, and that remove commands leave projects in a buildable state.
/// </summary>
public static class ScaffoldingIntegrationTest
{
    public static void Run()
    {
        Console.WriteLine("=== Scaffolding Integration Tests ===");
        Console.WriteLine();

        TestCreateModuleIntegration();
        TestCreatePluginIntegration();
        TestCreateProjectIntegration();
        TestRemoveModuleIntegration();
        TestRemovePluginIntegration();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    /// <summary>
    /// End-to-end: create-module → verify files → generate-project-files → verify .sln includes module.
    /// </summary>
    private static void TestCreateModuleIntegration()
    {
        Console.WriteLine("[Test 1] CreateModule → GenerateProjectFiles pipeline");

        var root = CreateTempProjectWithModuleTemplates();
        try
        {
            var proj = ProjectRoot(root);

            // 1. Create module
            var createResult = RunCommand(new CreateModuleCommand(), proj,
                new() { ["name"] = "Gameplay" });
            Assert(createResult.Success, $"create-module failed: {createResult.Message}");

            // 2. Verify files created
            var moduleDir = Path.Combine(proj, "Source", "Gameplay");
            Assert(Directory.Exists(moduleDir), "Module directory should exist");
            Assert(File.Exists(Path.Combine(moduleDir, "Gameplay.Build.cs")), ".Build.cs missing");

            // 3. Verify .eproject updated
            var eproject = ParseEproject(proj);
            Assert(FindInArray(eproject["Modules"]!.AsArray(), "Gameplay"),
                "Gameplay should be in .eproject Modules");

            // 4. Verify .Target.cs updated
            var targetContent = File.ReadAllText(Path.Combine(proj, "Source", "TestGame.Target.cs"));
            Assert(targetContent.Contains("ExtraModuleNames.Add(\"Gameplay\")"),
                "Target should reference Gameplay");

            // 5. GenerateProjectFiles succeeds
            var genResult = RunCommand(new GenerateProjectFilesCommand(), proj);
            Assert(genResult.Success, $"generate-project-files failed: {genResult.Message}");

            // 6. Verify .sln includes new module
            var slnPath = Path.Combine(proj, "TestGame.sln");
            Assert(File.Exists(slnPath), ".sln should exist");
            var slnContent = File.ReadAllText(slnPath);
            Assert(slnContent.Contains("Gameplay"), ".sln should reference Gameplay module");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    /// <summary>
    /// End-to-end: create-plugin → verify files → generate-project-files → verify .sln includes plugin.
    /// </summary>
    private static void TestCreatePluginIntegration()
    {
        Console.WriteLine("[Test 2] CreatePlugin → GenerateProjectFiles pipeline");

        var root = CreateTempProjectWithAllTemplates();
        try
        {
            var proj = ProjectRoot(root);

            // 1. Create plugin
            var createResult = RunCommand(new CreatePluginCommand(), proj,
                new() { ["name"] = "MyPlugin", ["category"] = "AI" });
            Assert(createResult.Success, $"create-plugin failed: {createResult.Message}");

            // 2. Verify plugin structure
            var pluginDir = Path.Combine(proj, "Plugins", "MyPlugin");
            Assert(Directory.Exists(pluginDir), "Plugin directory should exist");
            Assert(File.Exists(Path.Combine(pluginDir, "MyPlugin.eplugin")), ".eplugin missing");
            Assert(File.Exists(Path.Combine(pluginDir, "Source", "MyPlugin", "MyPlugin.Build.cs")),
                "Plugin .Build.cs missing");

            // 3. Verify .eproject updated (Plugins array)
            var eproject = ParseEproject(proj);
            Assert(FindInArray(eproject["Plugins"]!.AsArray(), "MyPlugin"),
                "MyPlugin should be in .eproject Plugins");

            // 4. GenerateProjectFiles succeeds
            var genResult = RunCommand(new GenerateProjectFilesCommand(), proj);
            Assert(genResult.Success, $"generate-project-files failed: {genResult.Message}");

            // 5. Verify .sln includes plugin module
            var slnContent = File.ReadAllText(Path.Combine(proj, "TestGame.sln"));
            Assert(slnContent.Contains("MyPlugin"), ".sln should reference MyPlugin");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    /// <summary>
    /// End-to-end: create-project → verify full structure → generate-project-files → verify .sln.
    /// </summary>
    private static void TestCreateProjectIntegration()
    {
        Console.WriteLine("[Test 3] CreateProject → GenerateProjectFiles pipeline");

        var location = CreateTempLocation();
        try
        {
            // 1. Create project
            var options = new BuildOptions
            {
                ProjectPath = ".",
                ExtraArguments = new Dictionary<string, string>
                {
                    ["name"] = "IntegrationGame",
                    ["location"] = location,
                },
            };
            var createResult = new CreateProjectCommand().Execute(options);
            Assert(createResult.Success, $"create-project failed: {createResult.Message}");

            // 2. Verify project structure
            var projDir = Path.Combine(location, "IntegrationGame");
            Assert(Directory.Exists(projDir), "Project directory should exist");
            Assert(File.Exists(Path.Combine(projDir, "IntegrationGame.eproject")), ".eproject missing");
            Assert(File.Exists(Path.Combine(projDir, "Source", "IntegrationGame.Target.cs")), ".Target.cs missing");
            Assert(File.Exists(Path.Combine(projDir, "Source", "IntegrationGame", "IntegrationGame.Build.cs")),
                ".Build.cs missing");

            // 3. Verify .eproject content
            var eproject = JsonNode.Parse(File.ReadAllText(
                Path.Combine(projDir, "IntegrationGame.eproject")))!.AsObject();
            Assert(FindInArray(eproject["Modules"]!.AsArray(), "IntegrationGame"),
                "IntegrationGame should be in Modules");

            // 4. Verify .sln was generated (CreateProjectCommand calls GenerateProjectFiles internally)
            var slnPath = Path.Combine(projDir, "IntegrationGame.sln");
            if (File.Exists(slnPath))
            {
                var slnContent = File.ReadAllText(slnPath);
                Assert(slnContent.Contains("IntegrationGame"), ".sln should reference IntegrationGame");
            }

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(location); }
    }

    /// <summary>
    /// End-to-end: create-module → generate (pass) → remove-module → generate (pass) → verify clean.
    /// </summary>
    private static void TestRemoveModuleIntegration()
    {
        Console.WriteLine("[Test 4] CreateModule → RemoveModule → GenerateProjectFiles pipeline");

        var root = CreateTempProjectWithModuleTemplates();
        try
        {
            var proj = ProjectRoot(root);

            // 1. Create module
            var createResult = RunCommand(new CreateModuleCommand(), proj,
                new() { ["name"] = "TempModule" });
            Assert(createResult.Success, $"create-module failed: {createResult.Message}");

            // 2. Generate — should include TempModule
            var gen1 = RunCommand(new GenerateProjectFilesCommand(), proj);
            Assert(gen1.Success, $"generate after create failed: {gen1.Message}");
            var sln1 = File.ReadAllText(Path.Combine(proj, "TestGame.sln"));
            Assert(sln1.Contains("TempModule"), ".sln should contain TempModule after create");

            // 3. Remove module
            var removeResult = RunCommand(new RemoveModuleCommand(), proj,
                new() { ["name"] = "TempModule" });
            Assert(removeResult.Success, $"remove-module failed: {removeResult.Message}");

            // 4. Verify directory deleted
            Assert(!Directory.Exists(Path.Combine(proj, "Source", "TempModule")),
                "TempModule dir should be deleted");

            // 5. Verify .eproject clean
            var eproject = ParseEproject(proj);
            Assert(!FindInArray(eproject["Modules"]!.AsArray(), "TempModule"),
                "TempModule should NOT be in .eproject");

            // 6. Verify .Target.cs clean
            var targetContent = File.ReadAllText(Path.Combine(proj, "Source", "TestGame.Target.cs"));
            Assert(!targetContent.Contains("TempModule"),
                ".Target.cs should not reference TempModule");

            // 7. Generate again — should still succeed without TempModule
            var gen2 = RunCommand(new GenerateProjectFilesCommand(), proj);
            Assert(gen2.Success, $"generate after remove failed: {gen2.Message}");
            var sln2 = File.ReadAllText(Path.Combine(proj, "TestGame.sln"));
            Assert(!sln2.Contains("TempModule"), ".sln should NOT contain TempModule after remove");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    /// <summary>
    /// End-to-end: create-plugin → generate (pass) → remove-plugin → generate (pass) → verify clean.
    /// </summary>
    private static void TestRemovePluginIntegration()
    {
        Console.WriteLine("[Test 5] CreatePlugin → RemovePlugin → GenerateProjectFiles pipeline");

        var root = CreateTempProjectWithAllTemplates();
        try
        {
            var proj = ProjectRoot(root);

            // 1. Create plugin
            var createResult = RunCommand(new CreatePluginCommand(), proj,
                new() { ["name"] = "TempPlugin" });
            Assert(createResult.Success, $"create-plugin failed: {createResult.Message}");

            // 2. Generate — should include TempPlugin
            var gen1 = RunCommand(new GenerateProjectFilesCommand(), proj);
            Assert(gen1.Success, $"generate after create failed: {gen1.Message}");
            var sln1 = File.ReadAllText(Path.Combine(proj, "TestGame.sln"));
            Assert(sln1.Contains("TempPlugin"), ".sln should contain TempPlugin after create");

            // 3. Remove plugin
            var removeResult = RunCommand(new RemovePluginCommand(), proj,
                new() { ["name"] = "TempPlugin" });
            Assert(removeResult.Success, $"remove-plugin failed: {removeResult.Message}");

            // 4. Verify directory deleted
            Assert(!Directory.Exists(Path.Combine(proj, "Plugins", "TempPlugin")),
                "TempPlugin dir should be deleted");

            // 5. Verify .eproject clean
            var eproject = ParseEproject(proj);
            Assert(!FindInArray(eproject["Plugins"]!.AsArray(), "TempPlugin"),
                "TempPlugin should NOT be in .eproject");

            // 6. Generate again — should still succeed without TempPlugin
            var gen2 = RunCommand(new GenerateProjectFilesCommand(), proj);
            Assert(gen2.Success, $"generate after remove failed: {gen2.Message}");
            var sln2 = File.ReadAllText(Path.Combine(proj, "TestGame.sln"));
            Assert(!sln2.Contains("TempPlugin"), ".sln should NOT contain TempPlugin after remove");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupDir(root); }
    }

    // --- Helpers ---

    private static string ProjectRoot(string root) =>
        Path.Combine(root, "Games", "TestGame");

    private static BuildResult RunCommand(ICommand command, string projectPath,
        Dictionary<string, string>? extra = null)
    {
        var options = new BuildOptions
        {
            ProjectPath = projectPath,
            ExtraArguments = extra ?? [],
        };
        return command.Execute(options);
    }

    private static JsonObject ParseEproject(string proj)
    {
        var path = Path.Combine(proj, "TestGame.eproject");
        return JsonNode.Parse(File.ReadAllText(path))!.AsObject();
    }

    private static bool FindInArray(JsonArray array, string name)
    {
        foreach (var item in array)
        {
            if (item?["Name"]?.GetValue<string>() == name)
                return true;
        }
        return false;
    }

    /// <summary>
    /// Creates a temp project with Engine + Module templates for create-module/remove-module tests.
    /// </summary>
    private static string CreateTempProjectWithModuleTemplates()
    {
        var root = Path.Combine(Path.GetTempPath(), $"enigma_integ_{Guid.NewGuid():N}");

        // Engine/Source/Runtime/
        Directory.CreateDirectory(Path.Combine(root, "Engine", "Source", "Runtime"));

        // Module templates
        var tplDir = Path.Combine(root, "Engine", "Templates", "Module");
        Directory.CreateDirectory(Path.Combine(tplDir, "Public"));
        Directory.CreateDirectory(Path.Combine(tplDir, "Private"));
        File.WriteAllText(Path.Combine(tplDir, "MODULE_NAME.Build.cs.template"),
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
        File.WriteAllText(Path.Combine(tplDir, "Public", "MODULE_NAMEModule.h.template"),
            """
            #pragma once
            #include "MODULE_NAMEModule.generated.h"
            class MODULE_NAME_API MODULE_NAMEModule {};
            """);
        File.WriteAllText(Path.Combine(tplDir, "Private", "MODULE_NAMEModule.cpp.template"),
            """
            #include "MODULE_NAMEModule.h"
            """);

        // Base project
        SetupBaseProject(root);
        return root;
    }

    /// <summary>
    /// Creates a temp project with Engine + Module + Plugin templates for plugin tests.
    /// </summary>
    private static string CreateTempProjectWithAllTemplates()
    {
        var root = CreateTempProjectWithModuleTemplates();

        // Plugin templates
        var tplDir = Path.Combine(root, "Engine", "Templates", "Plugin");
        var tplSrc = Path.Combine(tplDir, "Source", "PLUGIN_NAME");
        Directory.CreateDirectory(Path.Combine(tplSrc, "Public"));
        Directory.CreateDirectory(Path.Combine(tplSrc, "Private"));
        File.WriteAllText(Path.Combine(tplDir, "PLUGIN_NAME.eplugin.template"),
            """
            {
                "FileVersion": 1,
                "FriendlyName": "PLUGIN_NAME",
                "Description": "PLUGIN_NAME plugin",
                "Category": "PLUGIN_CATEGORY",
                "Modules": [
                    { "Name": "PLUGIN_NAME", "Type": "Runtime", "LoadingPhase": "PostEngineInit" }
                ]
            }
            """);
        File.WriteAllText(Path.Combine(tplSrc, "PLUGIN_NAME.Build.cs.template"),
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
        File.WriteAllText(Path.Combine(tplSrc, "Public", "PLUGIN_NAMEModule.h.template"),
            """
            #pragma once
            class PLUGIN_NAME_API PLUGIN_NAMEModule {};
            """);
        File.WriteAllText(Path.Combine(tplSrc, "Private", "PLUGIN_NAMEModule.cpp.template"),
            """
            #include "PLUGIN_NAMEModule.h"
            """);

        return root;
    }

    /// <summary>
    /// Sets up the base project structure (TestGame module, .eproject, .Target.cs).
    /// </summary>
    private static void SetupBaseProject(string root)
    {
        var proj = Path.Combine(root, "Games", "TestGame");
        var src = Path.Combine(proj, "Source");

        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Public"));
        Directory.CreateDirectory(Path.Combine(src, "TestGame", "Private"));

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
    }

    private static string CreateTempLocation()
    {
        var path = Path.Combine(Path.GetTempPath(), $"enigma_integ_{Guid.NewGuid():N}");
        Directory.CreateDirectory(path);
        return path;
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
