// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// Tests for automation test CMake target generation.
/// </summary>
public static class AutomationTestBuildGeneratorTest
{
    public static void Run()
    {
        Console.WriteLine("=== AutomationTestBuildGenerator Tests ===");
        Console.WriteLine();

        TestProductionCMakeFiltersPrivateTests();
        TestProductionCMakeIgnoresTestOnlyDependencies();
        TestGeneratesRunnerTarget();
        TestGeneratesRiderRunConfigurations();
        TestRejectsShippingConfiguration();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestProductionCMakeFiltersPrivateTests()
    {
        Console.WriteLine("[Test 1] Production CMake filters Private/Tests from module sources");

        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new()
            {
                ModuleName = "Core",
                ModuleDirectory = "C:/Project/Engine/Source/Runtime/Core",
            },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "TestProject",
            modules,
            resolve,
            "C:/Project",
            configuration: BuildConfiguration.Shipping);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        Assert(
            result.Content.Contains("list(FILTER CORE_SOURCES EXCLUDE REGEX \"/Private/Tests/\")"),
            "Core source list should filter Private/Tests");
        Assert(
            !result.Content.Contains("add_executable(AutomationTestRunner"),
            "Production CMakeGenerator should not emit AutomationTestRunner");

        Console.WriteLine("  PASSED");
    }

    private static void TestProductionCMakeIgnoresTestOnlyDependencies()
    {
        Console.WriteLine("[Test 2] Production CMake ignores test-only dependencies");

        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new()
            {
                ModuleName = "Core",
                ModuleDirectory = "C:/Project/Engine/Source/Runtime/Core",
                PublicDependencyModuleNames = { "RuntimeDep" },
                PublicTestDependencyModuleNames = { "AutomationTest" },
                PrivateTestDependencyModuleNames = { "TestHelper", "googletest" },
            },
            ["RuntimeDep"] = new()
            {
                ModuleName = "RuntimeDep",
                ModuleDirectory = "C:/Project/Engine/Source/Runtime/RuntimeDep",
            },
            ["AutomationTest"] = new()
            {
                ModuleName = "AutomationTest",
                ModuleDirectory = "C:/Project/Engine/Source/Developer/AutomationTest",
            },
            ["TestHelper"] = new()
            {
                ModuleName = "TestHelper",
                ModuleDirectory = "C:/Project/Engine/Source/Developer/TestHelper",
            },
            ["googletest"] = new()
            {
                ModuleName = "googletest",
                ModuleDirectory = "C:/Project/Engine/Source/ThirdParty/googletest",
                IsHeaderOnly = true,
            },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "TestProject",
            modules,
            resolve,
            "C:/Project",
            configuration: BuildConfiguration.Development);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        string coreLinks = ExtractTargetLinkBlock(result.Content, "Core");
        Assert(coreLinks.Contains("        RuntimeDep"), "Core should link production dependencies");
        Assert(!coreLinks.Contains("AutomationTest"), "Core should not link public test-only dependencies");
        Assert(!coreLinks.Contains("TestHelper"), "Core should not link private test-only dependencies");
        Assert(!coreLinks.Contains("googletest"), "Core should not link GoogleTest through production CMake");

        Console.WriteLine("  PASSED");
    }

    private static void TestGeneratesRunnerTarget()
    {
        Console.WriteLine("[Test 3] AutomationTestRunner target links framework, backend, and test-only deps");

        var modules = CreateAutomationModules();
        var sources = new List<AutomationTestSource>
        {
            new()
            {
                ModuleName = "Core",
                ModuleDirectory = "C:/Project/Engine/Source/Runtime/Core",
                SourceFilePath = "C:/Project/Engine/Source/Runtime/Core/Private/Tests/CoreAutomationTests.cpp",
                RelativeSourcePath = "Private/Tests/CoreAutomationTests.cpp",
                Owner = AutomationTestSourceOwner.Engine,
            },
        };

        var result = new AutomationTestBuildGenerator().Generate(new AutomationTestBuildGenerator.GenerateInput
        {
            ProjectName = "EnigmaEngine",
            RootPath = "C:/Project/Engine",
            EngineRoot = "C:/Project/Engine",
            Modules = modules,
            TestSources = sources,
        });

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var content = result.Content;

        Assert(content.Contains("add_library(Core ${CORE_SOURCES})"), "Core module target should be generated");
        Assert(content.Contains("add_executable(AutomationTestRunner"), "Runner executable should be generated");
        Assert(content.Contains("CoreAutomationTests.cpp"), "Runner should include module-local test source");
        Assert(
            content.Contains("OUTPUT_NAME \"AutomationTestRunner-Win64-Test\""),
            "Runner output name should be fixed and test-specific");
        Assert(
            content.Contains("target_compile_definitions(AutomationTestRunner PRIVATE ENIGMA_WITH_AUTOMATION_TESTS=1)"),
            "Runner should define ENIGMA_WITH_AUTOMATION_TESTS");
        Assert(
            content.Contains("target_compile_definitions(AutomationTest PRIVATE ENIGMA_WITH_AUTOMATION_TESTS=1)"),
            "AutomationTest module should enable bridge code in Test runner builds");
        Assert(
            content.Contains("\"C:/Project/Engine/Source/Runtime/Core/Private/Tests\""),
            "Runner should include module Private/Tests path");
        Assert(content.Contains("target_link_libraries(AutomationTestRunner"), "Runner should link libraries");
        Assert(content.Contains("        Core"), "Runner should link tested module");
        Assert(content.Contains("        AutomationTest"), "Runner should link AutomationTest");
        Assert(content.Contains("        nlohmann_json"), "Runner should link nlohmann_json for JSON reports");
        Assert(content.Contains("ENIGMA_GOOGLETEST_SOURCE_DIR"), "Runner build should add GoogleTest source");
        Assert(content.Contains("        gtest"), "Runner should link gtest");
        Assert(content.Contains("        gmock"), "Runner should link gmock");
        Assert(!content.Contains("        googletest"), "Runner should not link the googletest wrapper as a module");
        Assert(content.Contains("        TestHelper"), "Runner should link private test dependency");

        Console.WriteLine("  PASSED");
    }

    private static void TestGeneratesRiderRunConfigurations()
    {
        Console.WriteLine("[Test 4] Rider run configurations are generated for automation tests");

        string workspace = Path.Combine(Path.GetTempPath(), $"AutomationRiderRunConfigTest_{Guid.NewGuid():N}");
        try
        {
            var result = new RiderRunConfigurationGenerator().GenerateAutomationTestConfigs(
                new RiderRunConfigurationGenerator.GenerateInput
                {
                    OutputDirectory = workspace,
                    BuildToolProjectPath = "$PROJECT_DIR$/Engine/Source/Programs/BuildTool/Source/BuildTool/BuildTool.csproj",
                    WorkingDirectory = "$PROJECT_DIR$",
                    RootArgument = "$PROJECT_DIR$",
                    EngineMode = true,
                    ReportDirectory = "Intermediate/AutomationTest/Reports",
                });

            Assert(result.Success, $"Expected success, got error: {result.Error}");
            Assert(result.GeneratedCount == 3, $"Expected 3 run configs, got {result.GeneratedCount}");
            Assert(result.OutputPaths.Count == 3, $"Expected 3 output paths, got {result.OutputPaths.Count}");
            Assert(result.OutputPaths.All(File.Exists), "Every generated run config should exist");

            string fastContent = ReadRunConfig(workspace, "Enigma Automation Tests - Fast.run.xml");
            string filteredContent = ReadRunConfig(workspace, "Enigma Automation Tests - Filtered.run.xml");
            string listContent = ReadRunConfig(workspace, "Enigma Automation Tests - List.run.xml");

            Assert(fastContent.Contains("automation-test $PROJECT_DIR$ --engine --run --profile local-fast"),
                "Fast config should run local-fast engine tests");
            Assert(fastContent.Contains("--report Intermediate/AutomationTest/Reports"),
                "Fast config should include report path");
            Assert(filteredContent.Contains("--name-prefix System --allow-empty"),
                "Filtered config should include prefix filter and allow-empty");
            Assert(listContent.Contains("automation-test $PROJECT_DIR$ --engine --list --profile all-non-perf"),
                "List config should list all non-performance engine tests");

            Console.WriteLine("  PASSED");
        }
        finally
        {
            DeleteTempWorkspace(workspace);
        }
    }

    private static void TestRejectsShippingConfiguration()
    {
        Console.WriteLine("[Test 5] AutomationTestRunner generation rejects Shipping");

        var result = new AutomationTestBuildGenerator().Generate(new AutomationTestBuildGenerator.GenerateInput
        {
            ProjectName = "EnigmaEngine",
            RootPath = "C:/Project/Engine",
            EngineRoot = "C:/Project/Engine",
            Modules = CreateAutomationModules(),
            TestSources = [],
            Configuration = BuildConfiguration.Shipping,
        });

        Assert(!result.Success, "Shipping automation runner generation should fail");
        Assert(
            result.Error?.Contains("Test configuration", StringComparison.OrdinalIgnoreCase) == true,
            $"Error should mention Test configuration, got: {result.Error}");
        Assert(string.IsNullOrEmpty(result.Content), "Failed generation should not return CMake content");

        Console.WriteLine("  PASSED");
    }

    private static Dictionary<string, ModuleRules> CreateAutomationModules()
    {
        return new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new()
            {
                ModuleName = "Core",
                ModuleDirectory = "C:/Project/Engine/Source/Runtime/Core",
                PrivateTestDependencyModuleNames = { "TestHelper", "googletest" },
            },
            ["AutomationTest"] = new()
            {
                ModuleName = "AutomationTest",
                ModuleDirectory = "C:/Project/Engine/Source/Developer/AutomationTest",
                PublicDependencyModuleNames = { "Core" },
            },
            ["googletest"] = new()
            {
                ModuleName = "googletest",
                ModuleDirectory = "C:/Project/Engine/Source/ThirdParty/googletest",
                IsHeaderOnly = true,
            },
            ["nlohmann_json"] = new()
            {
                ModuleName = "nlohmann_json",
                ModuleDirectory = "C:/Project/Engine/Source/ThirdParty/nlohmann_json",
                IsHeaderOnly = true,
                PublicIncludePaths = { "include" },
            },
            ["TestHelper"] = new()
            {
                ModuleName = "TestHelper",
                ModuleDirectory = "C:/Project/Engine/Source/Developer/TestHelper",
                PublicDependencyModuleNames = { "Core" },
            },
        };
    }

    private static string ExtractTargetLinkBlock(string content, string targetName)
    {
        string marker = $"target_link_libraries({targetName}";
        int start = content.IndexOf(marker, StringComparison.Ordinal);
        Assert(start >= 0, $"Missing target_link_libraries block for {targetName}");

        int end = content.IndexOf("\r\n)", start, StringComparison.Ordinal);
        int closeLength = 3;
        if (end < 0)
        {
            end = content.IndexOf("\n)", start, StringComparison.Ordinal);
            closeLength = 2;
        }

        Assert(end >= 0, $"Unterminated target_link_libraries block for {targetName}");
        return content[start..(end + closeLength)];
    }

    private static string ReadRunConfig(string workspace, string fileName)
    {
        string path = Path.Combine(workspace, ".run", fileName);
        Assert(File.Exists(path), $"Run config should exist: {path}");
        return File.ReadAllText(path);
    }

    private static void DeleteTempWorkspace(string workspace)
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
