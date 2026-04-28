// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Build;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// Tests for engine-root and project-root automation test source discovery.
/// </summary>
public static class AutomationTestScannerTest
{
    public static void Run()
    {
        Console.WriteLine("=== AutomationTestScanner Tests ===");
        Console.WriteLine();

        TestEngineRootScanDoesNotRequireProjectDescriptor();
        TestProjectScanDiscoversEngineGameAndPluginTests();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestEngineRootScanDoesNotRequireProjectDescriptor()
    {
        Console.WriteLine("[Test 1] Engine-root scan discovers engine and engine plugin tests");

        string workspace = CreateWorkspace();
        try
        {
            string engineRoot = Path.Combine(workspace, "Engine");
            CreateModule(Path.Combine(engineRoot, "Source", "Runtime", "Core"), "Core", addTest: true);
            CreatePlugin(Path.Combine(engineRoot, "Plugins"), "EnginePlugin", addTest: true);

            var result = AutomationTestScanner.ScanEngine(workspace);

            Assert(result.EngineMode, "EngineMode should be true");
            Assert(result.ProjectRoot is null, "ProjectRoot should be null in engine mode");
            Assert(result.EngineRoot == Path.GetFullPath(engineRoot), "EngineRoot should resolve from repository root");
            Assert(result.AllModules.ContainsKey("Core"), "Core module should be discovered");
            Assert(result.AllModules.ContainsKey("EnginePlugin"), "Engine plugin module should be discovered");
            Assert(result.Sources.Count == 2, $"Expected 2 test sources, got {result.Sources.Count}");

            AssertSource(result, "Core", AutomationTestSourceOwner.Engine, null);
            AssertSource(result, "EnginePlugin", AutomationTestSourceOwner.Plugin, "EnginePlugin");
            AssertNoProductionSource(result);

            Console.WriteLine("  PASSED");
        }
        finally
        {
            DeleteWorkspace(workspace);
        }
    }

    private static void TestProjectScanDiscoversEngineGameAndPluginTests()
    {
        Console.WriteLine("[Test 2] Project scan discovers engine, game, project plugin, and engine plugin tests");

        string workspace = CreateWorkspace();
        try
        {
            string engineRoot = Path.Combine(workspace, "Engine");
            string projectRoot = Path.Combine(workspace, "TestGame");

            CreateModule(Path.Combine(engineRoot, "Source", "Runtime", "Core"), "Core", addTest: true);
            CreatePlugin(Path.Combine(engineRoot, "Plugins"), "EnginePlugin", addTest: true);

            CreateModule(Path.Combine(projectRoot, "Source", "TestGame"), "TestGame", addTest: true, "Core");
            CreateProjectPlugin(projectRoot, "ProjectPlugin", addTest: true);
            CreateProjectFiles(projectRoot);

            var result = AutomationTestScanner.ScanProject(projectRoot);

            Assert(!result.EngineMode, "EngineMode should be false");
            Assert(result.ProjectRoot == Path.GetFullPath(projectRoot), "ProjectRoot should resolve");
            Assert(result.EngineRoot == Path.GetFullPath(engineRoot), "EngineRoot should resolve");
            Assert(result.AllModules.ContainsKey("Core"), "Core module should be discovered");
            Assert(result.AllModules.ContainsKey("TestGame"), "Game module should be discovered");
            Assert(result.AllModules.ContainsKey("ProjectPlugin"), "Project plugin module should be discovered");
            Assert(result.AllModules.ContainsKey("EnginePlugin"), "Engine plugin module should be discovered");
            Assert(result.Sources.Count == 4, $"Expected 4 test sources, got {result.Sources.Count}");

            AssertSource(result, "Core", AutomationTestSourceOwner.Engine, null);
            AssertSource(result, "TestGame", AutomationTestSourceOwner.Game, null);
            AssertSource(result, "ProjectPlugin", AutomationTestSourceOwner.Plugin, "ProjectPlugin");
            AssertSource(result, "EnginePlugin", AutomationTestSourceOwner.Plugin, "EnginePlugin");
            AssertNoProductionSource(result);

            Console.WriteLine("  PASSED");
        }
        finally
        {
            DeleteWorkspace(workspace);
        }
    }

    private static void AssertSource(
        AutomationTestScanner.ScanResult result,
        string moduleName,
        AutomationTestSourceOwner owner,
        string? pluginName)
    {
        var source = result.Sources.SingleOrDefault(item => item.ModuleName == moduleName);
        if (source is null)
        {
            throw new Exception($"Assertion failed: Missing test source for module {moduleName}");
        }

        Assert(source.Owner == owner, $"{moduleName} owner should be {owner}");
        Assert(source.PluginName == pluginName, $"{moduleName} plugin name should be {pluginName ?? "<null>"}");
        Assert(source.RelativeSourcePath == $"Private/Tests/{moduleName}AutomationTests.cpp",
            $"{moduleName} relative path mismatch: {source.RelativeSourcePath}");
        Assert(File.Exists(source.SourceFilePath), $"{moduleName} source file should exist");
    }

    private static void AssertNoProductionSource(AutomationTestScanner.ScanResult result)
    {
        bool hasProductionSource = result.Sources.Any(source => source.RelativeSourcePath.EndsWith(
            "/Private/Production.cpp", StringComparison.Ordinal));
        Assert(!hasProductionSource, "Production source files should not be listed as automation test sources");
    }

    private static string CreateWorkspace()
    {
        string workspace = Path.Combine(Path.GetTempPath(), $"EnigmaAutomationScannerTest_{Guid.NewGuid():N}");
        Directory.CreateDirectory(workspace);
        return workspace;
    }

    private static void CreateProjectFiles(string projectRoot)
    {
        Directory.CreateDirectory(projectRoot);
        File.WriteAllText(Path.Combine(projectRoot, "TestGame.eproject"), """
            {
              "FileVersion": 1,
              "EngineAssociation": "Test",
              "Modules": [
                {
                  "Name": "TestGame",
                  "Type": "Runtime",
                  "LoadingPhase": "Default"
                }
              ],
              "Plugins": [
                {
                  "Name": "ProjectPlugin",
                  "Enabled": true
                },
                {
                  "Name": "EnginePlugin",
                  "Enabled": true
                }
              ]
            }
            """);

        File.WriteAllText(Path.Combine(projectRoot, "Source", "TestGame.Target.cs"), """
            using EnigmaEngine;

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

    private static void CreateProjectPlugin(string projectRoot, string pluginName, bool addTest)
    {
        CreatePlugin(Path.Combine(projectRoot, "Plugins"), pluginName, addTest);
    }

    private static void CreatePlugin(string pluginsRoot, string pluginName, bool addTest)
    {
        string pluginRoot = Path.Combine(pluginsRoot, pluginName);
        Directory.CreateDirectory(pluginRoot);

        File.WriteAllText(Path.Combine(pluginRoot, $"{pluginName}.eplugin"), $$"""
            {
              "FileVersion": 1,
              "FriendlyName": "{{pluginName}}",
              "Modules": [
                {
                  "Name": "{{pluginName}}",
                  "Type": "Runtime",
                  "LoadingPhase": "Default"
                }
              ]
            }
            """);

        CreateModule(Path.Combine(pluginRoot, "Source", pluginName), pluginName, addTest, "Core");
    }

    private static void CreateModule(string moduleDir, string moduleName, bool addTest, params string[] dependencies)
    {
        Directory.CreateDirectory(Path.Combine(moduleDir, "Public"));
        Directory.CreateDirectory(Path.Combine(moduleDir, "Private"));

        File.WriteAllText(Path.Combine(moduleDir, "Private", "Production.cpp"),
            $"// Production source for {moduleName}.{Environment.NewLine}");

        if (addTest)
        {
            Directory.CreateDirectory(Path.Combine(moduleDir, "Private", "Tests"));
            File.WriteAllText(
                Path.Combine(moduleDir, "Private", "Tests", $"{moduleName}AutomationTests.cpp"),
                $"// Automation test source for {moduleName}.{Environment.NewLine}");
        }

        var dependencyLines = dependencies
            .Select(dependency => $"        PublicDependencyModuleNames.Add(\"{dependency}\");");

        File.WriteAllText(Path.Combine(moduleDir, $"{moduleName}.Build.cs"), $$"""
            using EnigmaEngine;

            public class {{moduleName}} : ModuleRules
            {
                public {{moduleName}}(ReadOnlyTargetRules Target) : base(Target)
                {
                    PublicIncludePaths.Add("Public");
                    PrivateIncludePaths.Add("Private");
            {{string.Join(Environment.NewLine, dependencyLines)}}
                }
            }
            """);
    }

    private static void DeleteWorkspace(string workspace)
    {
        string fullWorkspace = Path.GetFullPath(workspace);
        string tempRoot = Path.GetFullPath(Path.GetTempPath());
        if (!fullWorkspace.StartsWith(tempRoot, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException($"Refusing to delete non-temp test workspace: {fullWorkspace}");
        }

        if (Directory.Exists(fullWorkspace))
        {
            Directory.Delete(fullWorkspace, recursive: true);
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
