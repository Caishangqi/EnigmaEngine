using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Utils;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for SolutionGenerator (.sln file generation).
/// </summary>
public static class SolutionGeneratorTest
{
    public static void Run()
    {
        Console.WriteLine("=== SolutionGenerator Smoke Tests ===");
        Console.WriteLine();

        TestSlnHeader();
        TestSolutionFolders();
        TestModuleProjectEntries();
        TestThreeConfigEntries();
        TestConfigNames();
        TestProjectConfigMapping();
        TestNestedProjects();
        TestDeterministic();
        TestProjectDependencies();
        TestThirdPartyFolder();
        TestPackageConfigBuildMapping();
        TestNoRulesFilesInSolution();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static (string tempDir, SolutionGenerator.GenerateInput input) CreateTestInput()
    {
        string tempDir = Path.Combine(Path.GetTempPath(), "EnigmaTest", $"Sln_{Guid.NewGuid():N}");
        Directory.CreateDirectory(tempDir);

        var engineModules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core", ModuleDirectory = Path.Combine(tempDir, "Engine", "Source", "Runtime", "Core") },
            ["Engine"] = new() { ModuleName = "Engine", ModuleDirectory = Path.Combine(tempDir, "Engine", "Source", "Runtime", "Engine") },
            ["Launch"] = new() { ModuleName = "Launch", ModuleDirectory = Path.Combine(tempDir, "Engine", "Source", "Runtime", "Launch") },
        };

        var gameModules = new Dictionary<string, ModuleRules>
        {
            ["EnigmaArcade"] = new() { ModuleName = "EnigmaArcade", ModuleDirectory = Path.Combine(tempDir, "EnigmaArcade", "Source", "EnigmaArcade") },
            ["ArcadeGameplay"] = new() { ModuleName = "ArcadeGameplay", ModuleDirectory = Path.Combine(tempDir, "EnigmaArcade", "Source", "ArcadeGameplay") },
        };

        var pluginModules = new Dictionary<string, ModuleRules>
        {
            ["ArcadeFeature"] = new() { ModuleName = "ArcadeFeature", ModuleDirectory = Path.Combine(tempDir, "EnigmaArcade", "Plugins", "ArcadeFeature", "Source", "ArcadeFeature") },
        };

        // Build adjacency list: Launch→[Engine,Core], Engine→[Core], Core→[]
        var adjacency = new Dictionary<string, List<string>>
        {
            ["Core"] = [],
            ["Engine"] = ["Core"],
            ["Launch"] = ["Engine", "Core"],
            ["EnigmaArcade"] = ["Engine", "Core"],
            ["ArcadeGameplay"] = ["Engine", "Core"],
            ["ArcadeFeature"] = ["Engine", "Core"],
        };

        var resolveResult = new DependencyResolver.ResolveResult
        {
            Success = true,
            BuildOrder = ["Core", "Engine", "Launch", "EnigmaArcade", "ArcadeGameplay", "ArcadeFeature"],
            AdjacencyList = adjacency,
        };

        var gameTarget = new TargetRules { TargetName = "EnigmaArcade", Type = TargetType.Game };

        var input = new SolutionGenerator.GenerateInput
        {
            ProjectName = "EnigmaArcade",
            ProjectRootPath = tempDir,
            EngineRootPath = Path.Combine(tempDir, "Engine"),
            EngineModules = engineModules,
            GameModules = gameModules,
            PluginModules = pluginModules,
            ResolveResult = resolveResult,
            GameTarget = gameTarget,
        };

        return (tempDir, input);
    }

    private static void TestSlnHeader()
    {
        Console.WriteLine("[Test 1] Header: Format Version 12.00, VS Version 17");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);
            Assert(content.Contains("Format Version 12.00"), "Missing Format Version 12.00");
            Assert(content.Contains("Visual Studio Version 17"), "Missing VS Version 17");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestSolutionFolders()
    {
        Console.WriteLine("[Test 2] Solution folders: Engine, Games, Plugins, Programs, Runtime");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);
            string folderGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";
            Assert(content.Contains($"Project(\"{folderGuid}\") = \"Engine\""), "Missing Engine folder");
            Assert(content.Contains($"Project(\"{folderGuid}\") = \"Games\""), "Missing Games folder");
            Assert(content.Contains($"Project(\"{folderGuid}\") = \"Plugins\""), "Missing Plugins folder");
            Assert(content.Contains($"Project(\"{folderGuid}\") = \"Programs\""), "Missing Programs folder");
            Assert(content.Contains($"Project(\"{folderGuid}\") = \"Runtime\""), "Missing Runtime folder");
            // Rules folder should no longer exist
            Assert(!content.Contains($"Project(\"{folderGuid}\") = \"Rules\","), "Rules folder should not exist");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestModuleProjectEntries()
    {
        Console.WriteLine("[Test 3] Module projects: C++ type GUID for each module");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);
            string cppGuid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
            foreach (var name in new[] { "Core", "Engine", "Launch", "EnigmaArcade", "ArcadeGameplay", "ArcadeFeature" })
                Assert(content.Contains($"Project(\"{cppGuid}\") = \"{name}\""), $"Missing C++ project: {name}");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestThreeConfigEntries()
    {
        Console.WriteLine("[Test 4] SolutionConfigurationPlatforms: exactly 4 entries");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);
            // Extract section
            int start = content.IndexOf("SolutionConfigurationPlatforms");
            int end = content.IndexOf("EndGlobalSection", start);
            string section = content[start..end];
            // Count config lines (lines with " = " pattern inside section)
            int count = section.Split('\n').Count(l => l.Contains(" = ") && l.Contains("|"));
            Assert(count == 4, $"Expected 4 config entries, got {count}");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestConfigNames()
    {
        Console.WriteLine("[Test 5] Config names: DebugGame Game, Development Game, Shipping Game");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);
            Assert(content.Contains("DebugGame Game|Win64"), "Missing DebugGame Game|Win64");
            Assert(content.Contains("Development Game|Win64"), "Missing Development Game|Win64");
            Assert(content.Contains("Shipping Game|Win64"), "Missing Shipping Game|Win64");
            Assert(content.Contains("Package Game|Win64"), "Missing Package Game|Win64");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestProjectConfigMapping()
    {
        Console.WriteLine("[Test 6] ProjectConfigurationPlatforms: all projects mapped to 3 configs");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);
            // Each of 6 modules should have ActiveCfg and Build.0 for each of 3 configs = 6 lines per project
            string coreGuid = $"{{{GuidGenerator.GenerateForProject("Core").ToString().ToUpperInvariant()}}}";
            int coreEntries = content.Split('\n').Count(l => l.Contains(coreGuid) && l.Contains("ActiveCfg"));
            Assert(coreEntries == 4, $"Core should have 4 ActiveCfg entries, got {coreEntries}");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestNestedProjects()
    {
        Console.WriteLine("[Test 7] NestedProjects: modules under correct parent folders");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);

            string coreGuid = $"{{{GuidGenerator.GenerateForProject("Core").ToString().ToUpperInvariant()}}}";
            string runtimeGuid = $"{{{GuidGenerator.GenerateForFolder("Engine/Source/Runtime").ToString().ToUpperInvariant()}}}";
            Assert(content.Contains($"{coreGuid} = {runtimeGuid}"), "Core should be nested under Engine/Source/Runtime");

            string arcadeGuid = $"{{{GuidGenerator.GenerateForProject("EnigmaArcade").ToString().ToUpperInvariant()}}}";
            string gameSrcGuid = $"{{{GuidGenerator.GenerateForFolder("Games/EnigmaArcade/Source").ToString().ToUpperInvariant()}}}";
            Assert(content.Contains($"{arcadeGuid} = {gameSrcGuid}"), "EnigmaArcade should be nested under Games/EnigmaArcade/Source");

            string featureGuid = $"{{{GuidGenerator.GenerateForProject("ArcadeFeature").ToString().ToUpperInvariant()}}}";
            string pluginsGuid = $"{{{GuidGenerator.GenerateForFolder("Plugins").ToString().ToUpperInvariant()}}}";
            Assert(content.Contains($"{featureGuid} = {pluginsGuid}"), "ArcadeFeature should be nested under Plugins");

            // Plugins folder should be nested under Games/EnigmaArcade (sibling of Source)
            string gameProjectGuid = $"{{{GuidGenerator.GenerateForFolder("Games/EnigmaArcade").ToString().ToUpperInvariant()}}}";
            Assert(content.Contains($"{pluginsGuid} = {gameProjectGuid}"), "Plugins should be nested under Games/EnigmaArcade");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestDeterministic()
    {
        Console.WriteLine("[Test 8] Deterministic: two runs produce identical output");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var gen = new SolutionGenerator();
            var r1 = gen.Generate(input);
            string content1 = File.ReadAllText(r1.OutputPath);
            var r2 = gen.Generate(input);
            string content2 = File.ReadAllText(r2.OutputPath);
            Assert(content1 == content2, "Two runs produced different output");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestProjectDependencies()
    {
        Console.WriteLine("[Test 9] Dependencies: Launch depends on Engine");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);

            // Launch project should have ProjectDependencies section
            string engineGuid = $"{{{GuidGenerator.GenerateForProject("Engine").ToString().ToUpperInvariant()}}}";
            // Find Launch project entry and check it contains engine GUID in dependencies
            int launchIdx = content.IndexOf("= \"Launch\"");
            Assert(launchIdx > 0, "Launch project entry not found");
            int endProjectIdx = content.IndexOf("EndProject", launchIdx);
            string launchSection = content[launchIdx..endProjectIdx];
            Assert(launchSection.Contains("ProjectDependencies"), "Launch should have ProjectDependencies");
            Assert(launchSection.Contains(engineGuid), $"Launch should depend on Engine ({engineGuid})");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestThirdPartyFolder()
    {
        Console.WriteLine("[Test 10] ThirdParty: folder, project entry, and config mapping");
        var (tempDir, baseInput) = CreateTestInput();
        var thirdPartyModules = new Dictionary<string, ModuleRules>
        {
            ["nlohmann_json"] = new()
            {
                ModuleName = "nlohmann_json",
                IsHeaderOnly = true,
                ModuleDirectory = Path.Combine(tempDir, "Engine", "Source", "ThirdParty", "nlohmann_json"),
            },
        };
        var input = new SolutionGenerator.GenerateInput
        {
            ProjectName = baseInput.ProjectName,
            ProjectRootPath = baseInput.ProjectRootPath,
            EngineRootPath = baseInput.EngineRootPath,
            EngineModules = baseInput.EngineModules,
            GameModules = baseInput.GameModules,
            PluginModules = baseInput.PluginModules,
            ThirdPartyModules = thirdPartyModules,
            ResolveResult = baseInput.ResolveResult,
            GameTarget = baseInput.GameTarget,
        };
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);

            // ThirdParty solution folder exists
            string folderGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";
            Assert(content.Contains($"Project(\"{folderGuid}\") = \"ThirdParty\""), "Missing ThirdParty folder");

            // nlohmann_json project entry exists with C++ type GUID
            string cppGuid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
            Assert(content.Contains($"Project(\"{cppGuid}\") = \"nlohmann_json\""), "Missing nlohmann_json project");

            // nlohmann_json nested under Engine/ThirdParty
            string jsonProjGuid = $"{{{GuidGenerator.GenerateForProject("nlohmann_json").ToString().ToUpperInvariant()}}}";
            string thirdPartyGuid = $"{{{GuidGenerator.GenerateForFolder("Engine/Source/ThirdParty").ToString().ToUpperInvariant()}}}";
            Assert(content.Contains($"{jsonProjGuid} = {thirdPartyGuid}"),
                "nlohmann_json should be nested under Engine/Source/ThirdParty");

            // nlohmann_json has ActiveCfg but no Build.0 (Utility project)
            Assert(content.Contains($"{jsonProjGuid}.Development Game|Win64.ActiveCfg"),
                "Missing ActiveCfg for nlohmann_json");
            Assert(!content.Contains($"{jsonProjGuid}.Development Game|Win64.Build.0"),
                "ThirdParty Utility project should not have Build.0");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestPackageConfigBuildMapping()
    {
        Console.WriteLine("[Test 11] Package config: Build.0 only for executable modules");
        var (tempDir, baseInput) = CreateTestInput();
        // Set ExtraModuleNames so EnigmaArcade is executable
        var gameTarget = new TargetRules
        {
            TargetName = "EnigmaArcade",
            Type = TargetType.Game,
            ExtraModuleNames = { "EnigmaArcade" },
        };
        var input = new SolutionGenerator.GenerateInput
        {
            ProjectName = baseInput.ProjectName,
            ProjectRootPath = baseInput.ProjectRootPath,
            EngineRootPath = baseInput.EngineRootPath,
            EngineModules = baseInput.EngineModules,
            GameModules = baseInput.GameModules,
            PluginModules = baseInput.PluginModules,
            ResolveResult = baseInput.ResolveResult,
            GameTarget = gameTarget,
        };
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);

            // EnigmaArcade (executable) should have Build.0 for Package config
            string arcadeGuid = $"{{{GuidGenerator.GenerateForProject("EnigmaArcade").ToString().ToUpperInvariant()}}}";
            Assert(content.Contains($"{arcadeGuid}.Package Game|Win64.Build.0"),
                "Executable module EnigmaArcade should have Build.0 for Package config");

            // Core (non-executable) should NOT have Build.0 for Package config
            string coreGuid = $"{{{GuidGenerator.GenerateForProject("Core").ToString().ToUpperInvariant()}}}";
            Assert(content.Contains($"{coreGuid}.Package Game|Win64.ActiveCfg"),
                "Core should have ActiveCfg for Package config");
            Assert(!content.Contains($"{coreGuid}.Package Game|Win64.Build.0"),
                "Non-executable module Core should NOT have Build.0 for Package config");

            // Core should still have Build.0 for other configs
            Assert(content.Contains($"{coreGuid}.Development Game|Win64.Build.0"),
                "Core should have Build.0 for Development config");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void TestNoRulesFilesInSolution()
    {
        Console.WriteLine("[Test 12] No Rules Files or Rules folder in solution");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var result = new SolutionGenerator().Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.OutputPath);

            Assert(!content.Contains("\"Rules Files\""), "Rules Files project should not exist");
            string folderGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";
            Assert(!content.Contains($"Project(\"{folderGuid}\") = \"Rules\""), "Rules folder should not exist");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static void Cleanup(string dir)
    {
        try { if (Directory.Exists(dir)) Directory.Delete(dir, recursive: true); }
        catch { /* Best effort */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
