// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Generators;

using System.Text;
using BuildTool.Analysis;
using BuildTool.Models;

/// <summary>
/// Generates the CMake target used by Enigma automation test runner builds.
/// </summary>
public sealed class AutomationTestBuildGenerator
{
    public const string RunnerTargetName = "AutomationTestRunner";
    private const string AutomationTestModuleName = "AutomationTest";
    private const string GoogleTestModuleName = "googletest";
    private const string JsonModuleName = "nlohmann_json";

    /// <summary>Input for automation test build generation.</summary>
    public sealed class GenerateInput
    {
        public required string ProjectName { get; init; }
        public required string RootPath { get; init; }
        public required string EngineRoot { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> Modules { get; init; }
        public required IReadOnlyList<AutomationTestSource> TestSources { get; init; }
        public BuildConfiguration Configuration { get; init; } = BuildConfiguration.Test;
        public string Platform { get; init; } = "Win64";
    }

    /// <summary>Result of automation test CMake generation.</summary>
    public sealed class GenerateResult
    {
        public required bool Success { get; init; }
        public string Content { get; init; } = string.Empty;
        public string? Error { get; init; }

        public static GenerateResult Ok(string content) => new() { Success = true, Content = content };
        public static GenerateResult Fail(string error) => new() { Success = false, Error = error };
    }

    /// <summary>
    /// Generate a complete CMakeLists.txt that contains regular module targets and
    /// one dedicated AutomationTestRunner executable target.
    /// </summary>
    public GenerateResult Generate(GenerateInput input)
    {
        if (input.Configuration != BuildConfiguration.Test)
        {
            return GenerateResult.Fail("Automation test runner targets can only be generated for Test configuration.");
        }

        if (!input.Modules.ContainsKey(AutomationTestModuleName))
        {
            return GenerateResult.Fail("Automation test runner requires the AutomationTest module.");
        }

        if (!input.Modules.ContainsKey(JsonModuleName))
        {
            return GenerateResult.Fail("Automation test runner requires the nlohmann_json module.");
        }

        var modules = FilterGeneratedModules(input.Modules);
        var resolveResult = new DependencyResolver().Resolve(modules);
        if (!resolveResult.Success)
        {
            return GenerateResult.Fail($"Dependency resolution failed: {resolveResult.Error}");
        }

        var baseResult = new CMakeGenerator().Generate(
            input.ProjectName,
            modules,
            resolveResult,
            input.RootPath,
            configuration: input.Configuration,
            platform: input.Platform);

        if (!baseResult.Success)
        {
            return GenerateResult.Fail(baseResult.Error ?? "CMake generation failed.");
        }

        var sb = new StringBuilder(baseResult.Content);
        WriteGoogleTestBackend(sb, input);
        WriteRunnerTarget(sb, input);
        return GenerateResult.Ok(sb.ToString());
    }

    private static Dictionary<string, ModuleRules> FilterGeneratedModules(
        IReadOnlyDictionary<string, ModuleRules> modules)
    {
        var result = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        foreach (var (name, rules) in modules)
        {
            if (name.Equals(GoogleTestModuleName, StringComparison.Ordinal))
            {
                continue;
            }

            result[name] = rules;
        }

        return result;
    }

    private static void WriteGoogleTestBackend(StringBuilder sb, GenerateInput input)
    {
        string googleTestSourceDir = $"{NormalizePath(input.EngineRoot)}/Source/ThirdParty/googletest/source";

        sb.AppendLine();
        sb.AppendLine("# GoogleTest backend");
        sb.AppendLine($"set(ENIGMA_GOOGLETEST_SOURCE_DIR \"{googleTestSourceDir}\")");
        sb.AppendLine("if(EXISTS \"${ENIGMA_GOOGLETEST_SOURCE_DIR}/CMakeLists.txt\")");
        sb.AppendLine("    add_subdirectory(");
        sb.AppendLine("        \"${ENIGMA_GOOGLETEST_SOURCE_DIR}\"");
        sb.AppendLine("        \"${CMAKE_BINARY_DIR}/ThirdParty/googletest\")");
        sb.AppendLine("endif()");
        sb.AppendLine("if(TARGET AutomationTest AND TARGET gtest)");
        sb.AppendLine("    target_compile_definitions(AutomationTest PRIVATE ENIGMA_WITH_AUTOMATION_TESTS=1)");
        sb.AppendLine("    target_link_libraries(AutomationTest PRIVATE gtest gmock)");
        sb.AppendLine("endif()");
    }

    private static void WriteRunnerTarget(StringBuilder sb, GenerateInput input)
    {
        string engineRoot = NormalizePath(input.EngineRoot);
        string runnerRoot = $"{engineRoot}/Source/Programs/AutomationTestRunner";
        string outputName = $"{RunnerTargetName}-{input.Platform}-{input.Configuration}";
        string sourceListName = "AUTOMATION_TEST_SOURCE_FILES";
        string runnerSourceListName = "AUTOMATION_TEST_RUNNER_SOURCES";

        sb.AppendLine();
        sb.AppendLine("# Automation test runner");
        sb.AppendLine($"set(AUTOMATION_TEST_RUNNER_ROOT \"{runnerRoot}\")");
        sb.AppendLine($"file(GLOB_RECURSE {runnerSourceListName} CONFIGURE_DEPENDS");
        sb.AppendLine("    \"${AUTOMATION_TEST_RUNNER_ROOT}/Private/*.cpp\"");
        sb.AppendLine("    \"${AUTOMATION_TEST_RUNNER_ROOT}/Public/*.h\"");
        sb.AppendLine("    \"${AUTOMATION_TEST_RUNNER_ROOT}/Public/*.hpp\"");
        sb.AppendLine(")");
        sb.AppendLine();

        sb.AppendLine($"set({sourceListName}");
        foreach (var source in input.TestSources.OrderBy(item => item.SourceFilePath, StringComparer.Ordinal))
        {
            sb.AppendLine($"    \"{NormalizePath(source.SourceFilePath)}\"");
        }
        sb.AppendLine(")");
        sb.AppendLine();

        sb.AppendLine($"add_executable({RunnerTargetName}");
        sb.AppendLine($"    ${{{runnerSourceListName}}}");
        sb.AppendLine($"    ${{{sourceListName}}}");
        sb.AppendLine(")");
        sb.AppendLine($"set_target_properties({RunnerTargetName} PROPERTIES OUTPUT_NAME \"{outputName}\")");
        sb.AppendLine($"target_compile_definitions({RunnerTargetName} PRIVATE ENIGMA_WITH_AUTOMATION_TESTS=1)");
        sb.AppendLine();

        WriteRunnerIncludePaths(sb, input, runnerRoot);
        WriteRunnerLinks(sb, input);
    }

    private static void WriteRunnerIncludePaths(StringBuilder sb, GenerateInput input, string runnerRoot)
    {
        var includePaths = new List<string>
        {
            $"{NormalizePath(input.EngineRoot)}/Intermediate/Generated",
            $"{runnerRoot}/Public",
            $"{runnerRoot}/Private",
        };

        foreach (var moduleName in GetSelectedModuleNames(input.TestSources))
        {
            if (!input.Modules.TryGetValue(moduleName, out var rules)
                || string.IsNullOrWhiteSpace(rules.ModuleDirectory))
            {
                continue;
            }

            string moduleDir = NormalizePath(rules.ModuleDirectory);
            AddUnique(includePaths, $"{moduleDir}/Public");
            AddUnique(includePaths, $"{moduleDir}/Private");
            AddUnique(includePaths, $"{moduleDir}/Private/Tests");
        }

        sb.AppendLine($"target_include_directories({RunnerTargetName}");
        sb.AppendLine("    PRIVATE");
        foreach (var includePath in includePaths)
        {
            sb.AppendLine($"        \"{includePath}\"");
        }
        sb.AppendLine(")");
        sb.AppendLine();
    }

    private static void WriteRunnerLinks(StringBuilder sb, GenerateInput input)
    {
        var linkNames = new List<string>();

        foreach (var moduleName in GetSelectedModuleNames(input.TestSources))
        {
            AddUnique(linkNames, moduleName);

            if (!input.Modules.TryGetValue(moduleName, out var rules))
            {
                continue;
            }

            AddRangeUnique(linkNames, rules.PublicTestDependencyModuleNames);
            AddRangeUnique(linkNames, rules.PrivateTestDependencyModuleNames);
        }

        AddUnique(linkNames, AutomationTestModuleName);
        AddUnique(linkNames, JsonModuleName);
        AddUnique(linkNames, "gtest");
        AddUnique(linkNames, "gmock");

        if (linkNames.Count == 0)
        {
            return;
        }

        sb.AppendLine($"target_link_libraries({RunnerTargetName}");
        sb.AppendLine("    PRIVATE");
        foreach (var linkName in linkNames)
        {
            sb.AppendLine($"        {linkName}");
        }
        sb.AppendLine(")");
    }

    private static IReadOnlyList<string> GetSelectedModuleNames(IReadOnlyList<AutomationTestSource> sources)
    {
        return sources
            .Select(source => source.ModuleName)
            .Distinct(StringComparer.Ordinal)
            .OrderBy(name => name, StringComparer.Ordinal)
            .ToList();
    }

    private static void AddRangeUnique(List<string> values, IEnumerable<string> candidates)
    {
        foreach (var candidate in candidates)
        {
            AddUnique(values, candidate);
        }
    }

    private static void AddUnique(List<string> values, string value)
    {
        if (!string.IsNullOrWhiteSpace(value) && !values.Contains(value, StringComparer.Ordinal))
        {
            if (value.Equals(GoogleTestModuleName, StringComparison.Ordinal))
            {
                return;
            }

            values.Add(value);
        }
    }

    private static string NormalizePath(string path)
    {
        return path.TrimEnd('/', '\\').Replace('\\', '/');
    }
}
