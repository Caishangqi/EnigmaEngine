// Copyright EnigmaEngine. All Rights Reserved.

using System.Xml.Linq;
using BuildTool.Commands;
using BuildTool.Models;
using BuildTool.Utils;

namespace BuildTool.Tests;

/// <summary>
/// Integration tests for GenerateProjectFilesCommand.
/// Runs the full pipeline on the real EnigmaArcade project.
/// </summary>
public static class GenerateProjectFilesCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== GenerateProjectFilesCommand Integration Tests ===");
        Console.WriteLine();

        TestFullGeneration();
        TestIncrementalUpdate();
        TestMissingProject();
        TestSlnContentValidation();
        TestThirdPartyModuleInSolution();
        TestThirdPartyVcxprojGenerated();
        TestThirdPartyVcxprojContainsHeaders();
        TestThirdPartyVcxprojContainsBuildCs();
        TestBuildCsInModuleVcxproj();
        TestTargetCsInModuleVcxproj();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestFullGeneration()
    {
        Console.WriteLine("[Test 1] Full generation: .sln and .vcxproj files created");
        var projectRoot = ResolveEnigmaArcadeRoot();
        var eprojectPath = Path.Combine(projectRoot, "EnigmaArcade.eproject");
        Assert(File.Exists(eprojectPath), $".eproject not found: {eprojectPath}");

        var command = new GenerateProjectFilesCommand();
        var options = new BuildOptions { ProjectPath = eprojectPath };
        var result = command.Execute(options);
        Assert(result.Success, $"Execute failed: {result.Message} {result.ErrorDetail}");

        // Verify .sln exists
        string slnPath = Path.Combine(projectRoot, "EnigmaArcade.sln");
        Assert(File.Exists(slnPath), $".sln not found: {slnPath}");

        // Verify .vcxproj files exist for known modules
        // Engine modules → Engine/Intermediate/ProjectFiles/
        string engineRoot = Path.GetFullPath(Path.Combine(projectRoot, "..", "Engine"));
        string engineIntermediateDir = Path.Combine(engineRoot, "Intermediate", "ProjectFiles");
        foreach (var module in new[] { "Core", "Engine", "Launch" })
        {
            string vcxproj = Path.Combine(engineIntermediateDir, $"{module}.vcxproj");
            Assert(File.Exists(vcxproj), $".vcxproj not found: {vcxproj}");

            string filters = Path.Combine(engineIntermediateDir, $"{module}.vcxproj.filters");
            Assert(File.Exists(filters), $".vcxproj.filters not found: {filters}");
        }

        // Game modules → Project/Intermediate/ProjectFiles/
        string gameIntermediateDir = Path.Combine(projectRoot, "Intermediate", "ProjectFiles");
        foreach (var module in new[] { "EnigmaArcade" })
        {
            string vcxproj = Path.Combine(gameIntermediateDir, $"{module}.vcxproj");
            Assert(File.Exists(vcxproj), $".vcxproj not found: {vcxproj}");

            string filters = Path.Combine(gameIntermediateDir, $"{module}.vcxproj.filters");
            Assert(File.Exists(filters), $".vcxproj.filters not found: {filters}");
        }

        // Plugin modules → Plugins/{PluginName}/Intermediate/ProjectFiles/
        string arcadeFeatureDir = Path.Combine(projectRoot, "Plugins", "ArcadeFeature", "Intermediate", "ProjectFiles");
        string afVcxproj = Path.Combine(arcadeFeatureDir, "ArcadeFeature.vcxproj");
        Assert(File.Exists(afVcxproj), $".vcxproj not found: {afVcxproj}");
        string afFilters = Path.Combine(arcadeFeatureDir, "ArcadeFeature.vcxproj.filters");
        Assert(File.Exists(afFilters), $".vcxproj.filters not found: {afFilters}");

        Console.WriteLine("  PASSED");
    }

    private static void TestIncrementalUpdate()
    {
        Console.WriteLine("[Test 2] Incremental: second run preserves unchanged files");
        var projectRoot = ResolveEnigmaArcadeRoot();
        var eprojectPath = Path.Combine(projectRoot, "EnigmaArcade.eproject");

        // First run (already done in Test 1, but run again to be safe)
        var command = new GenerateProjectFilesCommand();
        var options = new BuildOptions { ProjectPath = eprojectPath };
        var result1 = command.Execute(options);
        Assert(result1.Success, $"First run failed: {result1.Message}");

        // Record timestamps
        string slnPath = Path.Combine(projectRoot, "EnigmaArcade.sln");
        var slnTimeBefore = File.GetLastWriteTimeUtc(slnPath);

        string engineRoot = Path.GetFullPath(Path.Combine(projectRoot, "..", "Engine"));
        string engineIntermediateDir = Path.Combine(engineRoot, "Intermediate", "ProjectFiles");
        string coreVcxproj = Path.Combine(engineIntermediateDir, "Core.vcxproj");
        var coreTimeBefore = File.GetLastWriteTimeUtc(coreVcxproj);

        // Small delay to ensure filesystem timestamp granularity
        System.Threading.Thread.Sleep(100);

        // Second run
        var result2 = command.Execute(options);
        Assert(result2.Success, $"Second run failed: {result2.Message}");

        // Verify timestamps unchanged (AtomicFileWriter skipped identical content)
        var slnTimeAfter = File.GetLastWriteTimeUtc(slnPath);
        var coreTimeAfter = File.GetLastWriteTimeUtc(coreVcxproj);
        Assert(slnTimeBefore == slnTimeAfter,
            $".sln timestamp changed: {slnTimeBefore} → {slnTimeAfter}");
        Assert(coreTimeBefore == coreTimeAfter,
            $"Core.vcxproj timestamp changed: {coreTimeBefore} → {coreTimeAfter}");

        Console.WriteLine("  PASSED");
    }

    private static void TestMissingProject()
    {
        Console.WriteLine("[Test 3] Missing project: clear error message");
        var command = new GenerateProjectFilesCommand();
        var fakePath = Path.Combine(Path.GetTempPath(), "NonExistent", "Fake.eproject");
        var options = new BuildOptions { ProjectPath = fakePath };
        var result = command.Execute(options);
        Assert(!result.Success, "Expected failure for missing project");
        Assert(result.ErrorDetail is not null && result.ErrorDetail.Length > 0,
            "Expected error detail with searched paths");
        Console.WriteLine($"  Error (expected): {result.Message}");
        Console.WriteLine("  PASSED");
    }

    private static void TestSlnContentValidation()
    {
        Console.WriteLine("[Test 4] .sln content: format, projects, configs");
        var projectRoot = ResolveEnigmaArcadeRoot();
        string slnPath = Path.Combine(projectRoot, "EnigmaArcade.sln");
        Assert(File.Exists(slnPath), $".sln not found (run Test 1 first): {slnPath}");

        string content = File.ReadAllText(slnPath);

        // Header
        Assert(content.Contains("Format Version 12.00"), "Missing Format Version 12.00");
        Assert(content.Contains("Visual Studio Version 17"), "Missing VS Version 17");

        // Solution folders
        string folderGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";
        Assert(content.Contains($"Project(\"{folderGuid}\") = \"Engine\""), "Missing Engine folder");
        Assert(content.Contains($"Project(\"{folderGuid}\") = \"Games\""), "Missing Games folder");
        Assert(content.Contains($"Project(\"{folderGuid}\") = \"Plugins\""), "Missing Plugins folder");

        // C++ projects
        string cppGuid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
        foreach (var name in new[] { "Core", "Engine", "Launch", "EnigmaArcade", "ArcadeFeature" })
            Assert(content.Contains($"Project(\"{cppGuid}\") = \"{name}\""), $"Missing C++ project: {name}");

        // 4 solution configurations
        Assert(content.Contains("DebugGame Game|Win64"), "Missing DebugGame Game|Win64");
        Assert(content.Contains("Development Game|Win64"), "Missing Development Game|Win64");
        Assert(content.Contains("Shipping Game|Win64"), "Missing Shipping Game|Win64");
        Assert(content.Contains("Package Game|Win64"), "Missing Package Game|Win64");

        // Exactly 4 SolutionConfigurationPlatforms entries
        int start = content.IndexOf("SolutionConfigurationPlatforms");
        int end = content.IndexOf("EndGlobalSection", start);
        string section = content[start..end];
        int count = section.Split('\n').Count(l => l.Contains(" = ") && l.Contains("|"));
        Assert(count == 4, $"Expected 4 config entries, got {count}");

        // NestedProjects section exists
        Assert(content.Contains("NestedProjects"), "Missing NestedProjects section");

        Console.WriteLine("  PASSED");
    }

    private static void TestThirdPartyModuleInSolution()
    {
        Console.WriteLine("[Test 5] ThirdParty in .sln: nlohmann_json under ThirdParty folder");
        var projectRoot = ResolveEnigmaArcadeRoot();
        string slnPath = Path.Combine(projectRoot, "EnigmaArcade.sln");
        Assert(File.Exists(slnPath), $".sln not found: {slnPath}");

        string content = File.ReadAllText(slnPath);

        // ThirdParty solution folder exists
        string folderGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";
        Assert(content.Contains($"Project(\"{folderGuid}\") = \"ThirdParty\""),
            "Missing ThirdParty solution folder");

        // nlohmann_json project entry exists with C++ type GUID
        string cppGuid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
        Assert(content.Contains($"Project(\"{cppGuid}\") = \"nlohmann_json\""),
            "Missing nlohmann_json C++ project entry");

        // nlohmann_json nested under Engine/ThirdParty folder
        string jsonProjGuid = $"{{{GuidGenerator.GenerateForProject("nlohmann_json").ToString().ToUpperInvariant()}}}";
        string thirdPartyGuid = $"{{{GuidGenerator.GenerateForFolder("Engine/Source/ThirdParty").ToString().ToUpperInvariant()}}}";
        Assert(content.Contains($"{jsonProjGuid} = {thirdPartyGuid}"),
            "nlohmann_json should be nested under Engine/ThirdParty");

        // ActiveCfg present but no Build.0 (Utility project)
        Assert(content.Contains($"{jsonProjGuid}.Development Game|Win64.ActiveCfg"),
            "Missing ActiveCfg for nlohmann_json");
        Assert(!content.Contains($"{jsonProjGuid}.Development Game|Win64.Build.0"),
            "ThirdParty Utility project should not have Build.0");

        Console.WriteLine("  PASSED");
    }

    private static void TestThirdPartyVcxprojGenerated()
    {
        Console.WriteLine("[Test 6] ThirdParty .vcxproj: exists with ConfigurationType=Utility");
        var projectRoot = ResolveEnigmaArcadeRoot();
        string engineRoot = Path.GetFullPath(Path.Combine(projectRoot, "..", "Engine"));
        string engineIntermediateDir = Path.Combine(engineRoot, "Intermediate", "ProjectFiles");
        string vcxprojPath = Path.Combine(engineIntermediateDir, "nlohmann_json.vcxproj");
        Assert(File.Exists(vcxprojPath), $"nlohmann_json.vcxproj not found: {vcxprojPath}");

        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";
        var doc = XDocument.Load(vcxprojPath);

        // All 3 configs must be Utility
        var configTypes = doc.Descendants(ns + "ConfigurationType").ToList();
        Assert(configTypes.Count == 4, $"Expected 4 ConfigurationType, got {configTypes.Count}");
        foreach (var ct in configTypes)
            Assert(ct.Value == "Utility", $"Expected 'Utility', got '{ct.Value}'");

        // No NMake commands
        Assert(!doc.Descendants(ns + "NMakeBuildCommandLine").Any(),
            "Header-only .vcxproj should have no NMakeBuildCommandLine");
        Assert(!doc.Descendants(ns + "NMakeOutput").Any(),
            "Header-only .vcxproj should have no NMakeOutput");

        Console.WriteLine("  PASSED");
    }

    private static void TestThirdPartyVcxprojContainsHeaders()
    {
        Console.WriteLine("[Test 7] ThirdParty .vcxproj: json.hpp listed as ClInclude");
        var projectRoot = ResolveEnigmaArcadeRoot();
        string engineRoot = Path.GetFullPath(Path.Combine(projectRoot, "..", "Engine"));
        string vcxprojPath = Path.Combine(engineRoot, "Intermediate", "ProjectFiles", "nlohmann_json.vcxproj");
        Assert(File.Exists(vcxprojPath), $"nlohmann_json.vcxproj not found: {vcxprojPath}");

        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";
        var doc = XDocument.Load(vcxprojPath);

        var includes = doc.Descendants(ns + "ClInclude").ToList();
        Assert(includes.Count > 0, "Expected at least 1 ClInclude entry");
        Assert(includes.Any(i => (i.Attribute("Include")?.Value ?? "").Contains("json.hpp")),
            "Missing json.hpp in ClInclude items");

        Console.WriteLine("  PASSED");
    }

    private static void TestThirdPartyVcxprojContainsBuildCs()
    {
        Console.WriteLine("[Test 8] ThirdParty .vcxproj: Build.cs listed as None item");
        var projectRoot = ResolveEnigmaArcadeRoot();
        string engineRoot = Path.GetFullPath(Path.Combine(projectRoot, "..", "Engine"));
        string vcxprojPath = Path.Combine(engineRoot, "Intermediate", "ProjectFiles", "nlohmann_json.vcxproj");
        Assert(File.Exists(vcxprojPath), $"nlohmann_json.vcxproj not found: {vcxprojPath}");

        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";
        var doc = XDocument.Load(vcxprojPath);

        var noneItems = doc.Descendants(ns + "None").ToList();
        Assert(noneItems.Count > 0, "Expected at least 1 None item");
        Assert(noneItems.Any(i => (i.Attribute("Include")?.Value ?? "").Contains("nlohmann_json.Build.cs")),
            "Missing nlohmann_json.Build.cs in None items");

        Console.WriteLine("  PASSED");
    }

    private static void TestBuildCsInModuleVcxproj()
    {
        Console.WriteLine("[Test 9] Module .vcxproj: each compilable module contains its .Build.cs as None item");
        var projectRoot = ResolveEnigmaArcadeRoot();
        string engineRoot = Path.GetFullPath(Path.Combine(projectRoot, "..", "Engine"));
        string engineIntermediateDir = Path.Combine(engineRoot, "Intermediate", "ProjectFiles");
        string gameIntermediateDir = Path.Combine(projectRoot, "Intermediate", "ProjectFiles");

        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";

        // Check engine modules
        foreach (var module in new[] { "Core", "Engine" })
        {
            string vcxprojPath = Path.Combine(engineIntermediateDir, $"{module}.vcxproj");
            Assert(File.Exists(vcxprojPath), $"{module}.vcxproj not found: {vcxprojPath}");

            var doc = XDocument.Load(vcxprojPath);
            var noneItems = doc.Descendants(ns + "None").ToList();
            Assert(noneItems.Any(i => (i.Attribute("Include")?.Value ?? "").Contains(".Build.cs")),
                $"{module}.vcxproj should contain a .Build.cs as None item");
        }

        // Check game modules
        foreach (var module in new[] { "EnigmaArcade" })
        {
            string vcxprojPath = Path.Combine(gameIntermediateDir, $"{module}.vcxproj");
            Assert(File.Exists(vcxprojPath), $"{module}.vcxproj not found: {vcxprojPath}");

            var doc = XDocument.Load(vcxprojPath);
            var noneItems = doc.Descendants(ns + "None").ToList();
            Assert(noneItems.Any(i => (i.Attribute("Include")?.Value ?? "").Contains(".Build.cs")),
                $"{module}.vcxproj should contain a .Build.cs as None item");
        }

        Console.WriteLine("  PASSED");
    }

    private static void TestTargetCsInModuleVcxproj()
    {
        Console.WriteLine("[Test 10] Target.cs in module vcxproj: EnigmaArcade.Target.cs in EnigmaArcade, EnigmaGame.Target.cs in Launch");
        var projectRoot = ResolveEnigmaArcadeRoot();
        string engineRoot = Path.GetFullPath(Path.Combine(projectRoot, "..", "Engine"));
        string engineIntermediateDir = Path.Combine(engineRoot, "Intermediate", "ProjectFiles");
        string gameIntermediateDir = Path.Combine(projectRoot, "Intermediate", "ProjectFiles");

        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";

        // EnigmaArcade.Target.cs should be in EnigmaArcade.vcxproj (game side)
        string arcadeVcxproj = Path.Combine(gameIntermediateDir, "EnigmaArcade.vcxproj");
        Assert(File.Exists(arcadeVcxproj), $"EnigmaArcade.vcxproj not found");
        var arcadeDoc = XDocument.Load(arcadeVcxproj);
        var arcadeNone = arcadeDoc.Descendants(ns + "None").ToList();
        Assert(arcadeNone.Any(i => (i.Attribute("Include")?.Value ?? "").Contains("EnigmaArcade.Target.cs")),
            "EnigmaArcade.vcxproj should contain EnigmaArcade.Target.cs as None item");

        // EnigmaGame.Target.cs should be in Launch.vcxproj (engine side)
        string launchVcxproj = Path.Combine(engineIntermediateDir, "Launch.vcxproj");
        Assert(File.Exists(launchVcxproj), $"Launch.vcxproj not found");
        var launchDoc = XDocument.Load(launchVcxproj);
        var launchNone = launchDoc.Descendants(ns + "None").ToList();
        Assert(launchNone.Any(i => (i.Attribute("Include")?.Value ?? "").Contains("EnigmaGame.Target.cs")),
            "Launch.vcxproj should contain EnigmaGame.Target.cs as None item");

        // No Rules Files in .sln
        string slnPath = Path.Combine(projectRoot, "EnigmaArcade.sln");
        string slnContent = File.ReadAllText(slnPath);
        Assert(!slnContent.Contains("\"Rules Files\""), "Rules Files should not exist in .sln");

        Console.WriteLine("  PASSED");
    }

    private static string ResolveEnigmaArcadeRoot()
    {
        // Navigate from build output to EnigmaArcade
        var baseDir = AppContext.BaseDirectory;
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", "..", "EnigmaArcade"));

        if (Directory.Exists(candidate))
            return candidate;

        // Fallback: search upward for EnigmaEngine root
        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            var gameDir = Path.Combine(dir.FullName, "EnigmaArcade");
            if (Directory.Exists(gameDir))
                return gameDir;
            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException(
            "Cannot find EnigmaArcade directory. " +
            $"Searched from: {baseDir}");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
