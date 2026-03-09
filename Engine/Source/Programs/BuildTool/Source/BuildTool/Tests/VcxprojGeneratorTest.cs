using System.Xml.Linq;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Utils;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for VcxprojGenerator (.vcxproj and .vcxproj.filters).
/// </summary>
public static class VcxprojGeneratorTest
{
    private static readonly XNamespace Ns = "http://schemas.microsoft.com/developer/msbuild/2003";

    public static void Run()
    {
        Console.WriteLine("=== VcxprojGenerator Smoke Tests ===");
        Console.WriteLine();

        TestValidXml();
        TestThreeProjectConfigurations();
        TestMakefileType();
        TestDeterministicGuid();
        TestNMakeCommands();
        TestPreprocessorDefinitions();
        TestIncludePaths();
        TestSourceFiles();
        TestFiltersStructure();
        TestFilterGuidsDeterministic();
        TestNMakeOutput();
        TestNMakeOutputExecutable();
        TestPackageNMakeCommand();
        TestUserFile();
        TestHeaderOnlyUtilityType();
        TestHeaderOnlyNoNMake();
        TestBuildCsAsNoneItem();
        TestBuildCsInFilters();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static (string tempDir, VcxprojGenerator.ModuleProjectInput input) CreateTestInput()
    {
        string tempDir = Path.Combine(Path.GetTempPath(), "EnigmaTest", $"Vcxproj_{Guid.NewGuid():N}");
        string moduleDir = Path.Combine(tempDir, "Core");
        string outputDir = Path.Combine(tempDir, "Intermediate", "ProjectFiles");

        // Create source file structure
        Directory.CreateDirectory(Path.Combine(moduleDir, "Public", "Modules"));
        Directory.CreateDirectory(Path.Combine(moduleDir, "Private", "Modules"));

        File.WriteAllText(Path.Combine(moduleDir, "Public", "CoreAPI.h"), "// CoreAPI.h");
        File.WriteAllText(Path.Combine(moduleDir, "Public", "Modules", "ModuleManager.h"), "// ModuleManager.h");
        File.WriteAllText(Path.Combine(moduleDir, "Private", "CoreModule.cpp"), "// CoreModule.cpp");
        File.WriteAllText(Path.Combine(moduleDir, "Private", "Modules", "ModuleManager.cpp"), "// ModuleManager.cpp");

        var sourceFiles = new List<string>
        {
            Path.Combine(moduleDir, "Public", "CoreAPI.h"),
            Path.Combine(moduleDir, "Public", "Modules", "ModuleManager.h"),
            Path.Combine(moduleDir, "Private", "CoreModule.cpp"),
            Path.Combine(moduleDir, "Private", "Modules", "ModuleManager.cpp"),
        };

        var input = new VcxprojGenerator.ModuleProjectInput
        {
            ModuleName = "Core",
            Rules = new ModuleRules { ModuleName = "Core", ModuleDirectory = moduleDir },
            AllIncludePaths = [Path.Combine(moduleDir, "Public"), Path.Combine(tempDir, "Engine", "Public")],
            PreprocessorDefinitions = ["WIN32", "_WINDOWS"],
            SourceFiles = sourceFiles,
            ModuleSourceRoot = moduleDir,
            OutputDirectory = outputDir,
            BuildToolCsprojPath = @"C:\BuildTool\BuildTool.csproj",
            ProjectFilePath = @"C:\Projects\EnigmaArcade.eproject",
            IsExecutable = false,
            ProjectName = "TestProject",
        };

        return (tempDir, input);
    }

    /// <summary>Parse generated .vcxproj as valid XML.</summary>
    private static void TestValidXml()
    {
        Console.WriteLine("[Test 1] Valid XML: .vcxproj parses without errors");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);
            Assert(doc.Root is not null, "Root element is null");
            Assert(doc.Root!.Name.LocalName == "Project", $"Root: expected 'Project', got '{doc.Root.Name.LocalName}'");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Verify exactly 3 ProjectConfiguration entries.</summary>
    private static void TestThreeProjectConfigurations()
    {
        Console.WriteLine("[Test 2] 4 ProjectConfigurations");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);
            var configs = doc.Descendants(Ns + "ProjectConfiguration").ToList();
            Assert(configs.Count == 4, $"Expected 4 ProjectConfigurations, got {configs.Count}");
            var includes = configs.Select(c => c.Attribute("Include")?.Value).ToList();
            Assert(includes.Contains("DebugGame Game|Win64"), "Missing DebugGame Game|Win64");
            Assert(includes.Contains("Development Game|Win64"), "Missing Development Game|Win64");
            Assert(includes.Contains("Shipping Game|Win64"), "Missing Shipping Game|Win64");
            Assert(includes.Contains("Package Game|Win64"), "Missing Package Game|Win64");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>All configs must have ConfigurationType=Makefile.</summary>
    private static void TestMakefileType()
    {
        Console.WriteLine("[Test 3] ConfigurationType=Makefile for all configs");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);
            var configTypes = doc.Descendants(Ns + "ConfigurationType").ToList();
            Assert(configTypes.Count == 4, $"Expected 4 ConfigurationType, got {configTypes.Count}");
            foreach (var ct in configTypes)
                Assert(ct.Value == "Makefile", $"Expected 'Makefile', got '{ct.Value}'");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Same module name must produce same ProjectGuid.</summary>
    private static void TestDeterministicGuid()
    {
        Console.WriteLine("[Test 4] Deterministic GUID: same module -> same GUID");
        var (tempDir1, input1) = CreateTestInput();
        var (tempDir2, input2) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var r1 = generator.Generate(input1);
            var r2 = generator.Generate(input2);
            Assert(r1.Success && r2.Success, "Generate failed");
            var doc1 = XDocument.Load(r1.VcxprojPath);
            var doc2 = XDocument.Load(r2.VcxprojPath);
            string guid1 = doc1.Descendants(Ns + "ProjectGuid").First().Value;
            string guid2 = doc2.Descendants(Ns + "ProjectGuid").First().Value;
            Assert(guid1 == guid2, $"GUIDs differ: {guid1} vs {guid2}");
            Console.WriteLine($"  PASSED ({guid1})");
        }
        finally { Cleanup(tempDir1); Cleanup(tempDir2); }
    }

    /// <summary>NMake commands must contain build/rebuild/clean.</summary>
    private static void TestNMakeCommands()
    {
        Console.WriteLine("[Test 5] NMake commands: build, rebuild, clean");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.VcxprojPath);
            Assert(content.Contains("-- build"), "Missing build command");
            Assert(content.Contains("-- rebuild"), "Missing rebuild command");
            Assert(content.Contains("-- clean"), "Missing clean command");
            Assert(content.Contains("BuildTool.csproj"), "Missing BuildTool.csproj reference");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Preprocessor definitions must include MODULE_EXPORTS and config macros.</summary>
    private static void TestPreprocessorDefinitions()
    {
        Console.WriteLine("[Test 6] Preprocessor: CORE_EXPORTS + config macros");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.VcxprojPath);
            Assert(content.Contains("CORE_EXPORTS"), "Missing CORE_EXPORTS");
            Assert(content.Contains("ENIGMA_BUILD_DEVELOPMENT=1"), "Missing ENIGMA_BUILD_DEVELOPMENT=1");
            Assert(content.Contains("ENIGMA_BUILD_DEBUGGAME=1"), "Missing ENIGMA_BUILD_DEBUGGAME=1");
            Assert(content.Contains("ENIGMA_BUILD_SHIPPING=1"), "Missing ENIGMA_BUILD_SHIPPING=1");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Include paths must contain all provided paths.</summary>
    private static void TestIncludePaths()
    {
        Console.WriteLine("[Test 7] Include paths: all provided paths present");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.VcxprojPath);
            foreach (var path in input.AllIncludePaths)
                Assert(content.Contains(path), $"Missing include path: {path}");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Source files: .cpp as ClCompile, .h as ClInclude.</summary>
    private static void TestSourceFiles()
    {
        Console.WriteLine("[Test 8] Source files: ClCompile/.cpp, ClInclude/.h");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);
            var compiles = doc.Descendants(Ns + "ClCompile").ToList();
            var includes = doc.Descendants(Ns + "ClInclude").ToList();
            Assert(compiles.Count == 2, $"Expected 2 ClCompile, got {compiles.Count}");
            Assert(includes.Count == 2, $"Expected 2 ClInclude, got {includes.Count}");
            var cppPaths = compiles.Select(c => c.Attribute("Include")?.Value ?? "").ToList();
            Assert(cppPaths.Any(p => p.Contains("CoreModule.cpp")), "Missing CoreModule.cpp");
            Assert(cppPaths.Any(p => p.Contains("ModuleManager.cpp")), "Missing ModuleManager.cpp");
            var hPaths = includes.Select(c => c.Attribute("Include")?.Value ?? "").ToList();
            Assert(hPaths.Any(p => p.Contains("CoreAPI.h")), "Missing CoreAPI.h");
            Assert(hPaths.Any(p => p.Contains("ModuleManager.h")), "Missing ModuleManager.h");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Filters file must contain Filter entries for Public and Private.</summary>
    private static void TestFiltersStructure()
    {
        Console.WriteLine("[Test 9] Filters: Public and Private directories");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.FiltersPath);
            var filters = doc.Descendants(Ns + "Filter")
                .Select(f => f.Attribute("Include")?.Value ?? "").ToList();
            Assert(filters.Contains("Public"), "Missing 'Public' filter");
            Assert(filters.Contains("Private"), "Missing 'Private' filter");
            Assert(filters.Any(f => f.Contains("Modules")), "Missing 'Modules' sub-filter");
            Console.WriteLine($"  PASSED (filters: {string.Join(", ", filters)})");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Filter GUIDs must be deterministic.</summary>
    private static void TestFilterGuidsDeterministic()
    {
        Console.WriteLine("[Test 10] Filter GUIDs: deterministic across runs");
        var (tempDir1, input1) = CreateTestInput();
        var (tempDir2, input2) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var r1 = generator.Generate(input1);
            var r2 = generator.Generate(input2);
            Assert(r1.Success && r2.Success, "Generate failed");
            var doc1 = XDocument.Load(r1.FiltersPath);
            var doc2 = XDocument.Load(r2.FiltersPath);
            var guids1 = doc1.Descendants(Ns + "UniqueIdentifier").Select(e => e.Value).ToList();
            var guids2 = doc2.Descendants(Ns + "UniqueIdentifier").Select(e => e.Value).ToList();
            Assert(guids1.Count == guids2.Count, $"Count mismatch: {guids1.Count} vs {guids2.Count}");
            for (int i = 0; i < guids1.Count; i++)
                Assert(guids1[i] == guids2[i], $"Filter GUID[{i}] differs");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir1); Cleanup(tempDir2); }
    }

    /// <summary>NMakeOutput for library module: .dll paths with correct naming.</summary>
    private static void TestNMakeOutput()
    {
        Console.WriteLine("[Test 11] NMakeOutput: library module produces .dll paths");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);
            var outputs = doc.Descendants(Ns + "NMakeOutput").ToList();
            Assert(outputs.Count == 4, $"Expected 4 NMakeOutput, got {outputs.Count}");
            // Development config should produce {ProjectName}-{ModuleName}.dll
            Assert(outputs.Any(o => o.Value.Contains("EnigmaEngine-Core.dll")),
                "Missing Development DLL: EnigmaEngine-Core.dll");
            // DebugGame config should produce {ProjectName}-{ModuleName}-Win64-{Config}.dll
            Assert(outputs.Any(o => o.Value.Contains("EnigmaEngine-Core-Win64-DebugGame.dll")),
                "Missing DebugGame DLL: EnigmaEngine-Core-Win64-DebugGame.dll");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>NMakeOutput for executable module: .exe paths.</summary>
    private static void TestNMakeOutputExecutable()
    {
        Console.WriteLine("[Test 12] NMakeOutput: executable module produces .exe paths");
        var (tempDir, input) = CreateTestInput();
        // Create a new input with IsExecutable = true
        var exeInput = new VcxprojGenerator.ModuleProjectInput
        {
            ModuleName = input.ModuleName,
            Rules = input.Rules,
            AllIncludePaths = input.AllIncludePaths,
            PreprocessorDefinitions = input.PreprocessorDefinitions,
            SourceFiles = input.SourceFiles,
            ModuleSourceRoot = input.ModuleSourceRoot,
            OutputDirectory = input.OutputDirectory,
            BuildToolCsprojPath = input.BuildToolCsprojPath,
            ProjectFilePath = input.ProjectFilePath,
            IsExecutable = true,
            ProjectName = input.ProjectName,
        };
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(exeInput);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);
            var outputs = doc.Descendants(Ns + "NMakeOutput").ToList();
            Assert(outputs.Count == 4, $"Expected 4 NMakeOutput, got {outputs.Count}");
            // Development: {ProjectName}.exe (executable uses project name, not module name)
            Assert(outputs.Any(o => o.Value.Contains("EnigmaEngine.exe")),
                "Missing Development EXE: EnigmaEngine.exe");
            // DebugGame: {ProjectName}-Win64-{Config}.exe
            Assert(outputs.Any(o => o.Value.Contains("EnigmaEngine-Win64-DebugGame.exe")),
                "Missing DebugGame EXE: EnigmaEngine-Win64-DebugGame.exe");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Package config NMake commands must use 'package' instead of 'build'.</summary>
    private static void TestPackageNMakeCommand()
    {
        Console.WriteLine("[Test 13] Package config: NMake uses 'package' command");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            string content = File.ReadAllText(result.VcxprojPath);

            // Extract the Package config NMake PropertyGroup (second occurrence of Package Game|Win64)
            int firstOccurrence = content.IndexOf("'Package Game|Win64'");
            Assert(firstOccurrence > 0, "Package Game|Win64 PropertyGroup not found");
            int pkgStart = content.IndexOf("'Package Game|Win64'", firstOccurrence + 1);
            Assert(pkgStart > 0, "Package Game|Win64 NMake PropertyGroup not found");
            int pkgEnd = content.IndexOf("</PropertyGroup>", pkgStart);
            string pkgSection = content[pkgStart..pkgEnd];

            Assert(pkgSection.Contains("-- package --project"), "Package config NMakeBuild should use 'package' command");
            Assert(!pkgSection.Contains("-- build --project"), "Package config should NOT use 'build' command");
            Assert(pkgSection.Contains("-- clean --project"), "Package config NMakeClean should use 'clean' command");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>.vcxproj.user file with DebuggerFlavor for all configs.</summary>
    private static void TestUserFile()
    {
        Console.WriteLine("[Test 14] .vcxproj.user: DebuggerFlavor=WindowsLocalDebugger");
        var (tempDir, input) = CreateTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(input);
            Assert(result.Success, $"Generate failed: {result.Error}");
            Assert(File.Exists(result.UserFilePath), $".vcxproj.user not found: {result.UserFilePath}");
            var doc = XDocument.Load(result.UserFilePath);
            var flavors = doc.Descendants(Ns + "DebuggerFlavor").ToList();
            Assert(flavors.Count == 4, $"Expected 4 DebuggerFlavor, got {flavors.Count}");
            foreach (var f in flavors)
                Assert(f.Value == "WindowsLocalDebugger", $"Expected 'WindowsLocalDebugger', got '{f.Value}'");
            var workDirs = doc.Descendants(Ns + "LocalDebuggerWorkingDirectory").ToList();
            Assert(workDirs.Count == 4, $"Expected 4 LocalDebuggerWorkingDirectory, got {workDirs.Count}");
            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Header-only .vcxproj must have ConfigurationType=Utility.</summary>
    private static void TestHeaderOnlyUtilityType()
    {
        Console.WriteLine("[Test 15] Header-only: ConfigurationType=Utility");
        var (tempDir, hoInput) = CreateHeaderOnlyTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.GenerateHeaderOnly(hoInput);
            Assert(result.Success, $"GenerateHeaderOnly failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);

            // ConfigurationType must be Utility
            var configTypes = doc.Descendants(Ns + "ConfigurationType").ToList();
            Assert(configTypes.Count == 4, $"Expected 4 ConfigurationType, got {configTypes.Count}");
            foreach (var ct in configTypes)
                Assert(ct.Value == "Utility", $"Expected 'Utility', got '{ct.Value}'");

            // Build.cs as None item
            var noneItems = doc.Descendants(Ns + "None").ToList();
            Assert(noneItems.Count == 1, $"Expected 1 None item (Build.cs), got {noneItems.Count}");
            Assert(noneItems[0].Attribute("Include")?.Value.Contains("Build.cs") == true, "None item should be Build.cs");

            // Header as ClInclude
            var includes = doc.Descendants(Ns + "ClInclude").ToList();
            Assert(includes.Count == 1, $"Expected 1 ClInclude, got {includes.Count}");
            Assert(includes[0].Attribute("Include")?.Value.Contains("json.hpp") == true, "ClInclude should be json.hpp");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Header-only .vcxproj must NOT have NMake build commands or NMakeOutput.</summary>
    private static void TestHeaderOnlyNoNMake()
    {
        Console.WriteLine("[Test 16] Header-only: no NMake commands");
        var (tempDir, hoInput) = CreateHeaderOnlyTestInput();
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.GenerateHeaderOnly(hoInput);
            Assert(result.Success, $"GenerateHeaderOnly failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);

            Assert(!doc.Descendants(Ns + "NMakeBuildCommandLine").Any(), "Header-only should have no NMakeBuildCommandLine");
            Assert(!doc.Descendants(Ns + "NMakeReBuildCommandLine").Any(), "Header-only should have no NMakeReBuildCommandLine");
            Assert(!doc.Descendants(Ns + "NMakeCleanCommandLine").Any(), "Header-only should have no NMakeCleanCommandLine");
            Assert(!doc.Descendants(Ns + "NMakeOutput").Any(), "Header-only should have no NMakeOutput");

            // Filters file should exist and have filter entries
            Assert(File.Exists(result.FiltersPath), $".vcxproj.filters not found: {result.FiltersPath}");
            var filtersDoc = XDocument.Load(result.FiltersPath);
            var filterEntries = filtersDoc.Descendants(Ns + "Filter").ToList();
            Assert(filterEntries.Count > 0, "Filters file should have filter entries for header subdirectories");

            // No .vcxproj.user generated
            Assert(string.IsNullOrEmpty(result.UserFilePath), "Header-only should not produce .vcxproj.user");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Compilable module .vcxproj must include .Build.cs as a None item when BuildCsPath is set.</summary>
    private static void TestBuildCsAsNoneItem()
    {
        Console.WriteLine("[Test 17] Build.cs as None item in compilable module .vcxproj");
        var (tempDir, input) = CreateTestInput();
        // Create a .Build.cs file and set BuildCsPath
        string buildCsPath = Path.Combine(input.ModuleSourceRoot, "Core.Build.cs");
        File.WriteAllText(buildCsPath, "// Core.Build.cs");
        var inputWithBuildCs = new VcxprojGenerator.ModuleProjectInput
        {
            ModuleName = input.ModuleName,
            Rules = input.Rules,
            AllIncludePaths = input.AllIncludePaths,
            PreprocessorDefinitions = input.PreprocessorDefinitions,
            SourceFiles = input.SourceFiles,
            ModuleSourceRoot = input.ModuleSourceRoot,
            OutputDirectory = input.OutputDirectory,
            BuildToolCsprojPath = input.BuildToolCsprojPath,
            ProjectFilePath = input.ProjectFilePath,
            IsExecutable = input.IsExecutable,
            ProjectName = input.ProjectName,
            BuildCsPath = buildCsPath,
        };
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(inputWithBuildCs);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var doc = XDocument.Load(result.VcxprojPath);

            var noneItems = doc.Descendants(Ns + "None").ToList();
            Assert(noneItems.Count == 1, $"Expected 1 None item (Build.cs), got {noneItems.Count}");
            Assert(noneItems[0].Attribute("Include")?.Value.Contains("Core.Build.cs") == true,
                "None item should reference Core.Build.cs");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    /// <summary>Build.cs in .vcxproj.filters must appear at root level (no Filter element).</summary>
    private static void TestBuildCsInFilters()
    {
        Console.WriteLine("[Test 18] Build.cs in filters at root level (no filter path)");
        var (tempDir, input) = CreateTestInput();
        string buildCsPath = Path.Combine(input.ModuleSourceRoot, "Core.Build.cs");
        File.WriteAllText(buildCsPath, "// Core.Build.cs");
        var inputWithBuildCs = new VcxprojGenerator.ModuleProjectInput
        {
            ModuleName = input.ModuleName,
            Rules = input.Rules,
            AllIncludePaths = input.AllIncludePaths,
            PreprocessorDefinitions = input.PreprocessorDefinitions,
            SourceFiles = input.SourceFiles,
            ModuleSourceRoot = input.ModuleSourceRoot,
            OutputDirectory = input.OutputDirectory,
            BuildToolCsprojPath = input.BuildToolCsprojPath,
            ProjectFilePath = input.ProjectFilePath,
            IsExecutable = input.IsExecutable,
            ProjectName = input.ProjectName,
            BuildCsPath = buildCsPath,
        };
        try
        {
            var generator = new VcxprojGenerator();
            var result = generator.Generate(inputWithBuildCs);
            Assert(result.Success, $"Generate failed: {result.Error}");
            var filtersDoc = XDocument.Load(result.FiltersPath);

            // Build.cs should appear as <None> in filters
            var noneItems = filtersDoc.Descendants(Ns + "None").ToList();
            Assert(noneItems.Count == 1, $"Expected 1 None item in filters, got {noneItems.Count}");
            Assert(noneItems[0].Attribute("Include")?.Value.Contains("Core.Build.cs") == true,
                "Filters None item should reference Core.Build.cs");

            // None item should NOT have a <Filter> child (root level)
            var filterChild = noneItems[0].Element(Ns + "Filter");
            Assert(filterChild is null, "Build.cs in filters should not have a <Filter> child (root level)");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(tempDir); }
    }

    private static (string tempDir, VcxprojGenerator.HeaderOnlyProjectInput input) CreateHeaderOnlyTestInput()
    {
        string tempDir = Path.Combine(Path.GetTempPath(), "EnigmaTest", $"Vcxproj_{Guid.NewGuid():N}");
        string moduleDir = Path.Combine(tempDir, "nlohmann_json");
        string outputDir = Path.Combine(tempDir, "Intermediate", "ProjectFiles");

        Directory.CreateDirectory(Path.Combine(moduleDir, "include", "nlohmann"));
        File.WriteAllText(Path.Combine(moduleDir, "include", "nlohmann", "json.hpp"), "// json.hpp");
        File.WriteAllText(Path.Combine(moduleDir, "nlohmann_json.Build.cs"), "// Build.cs");

        var input = new VcxprojGenerator.HeaderOnlyProjectInput
        {
            ModuleName = "nlohmann_json",
            ModuleDirectory = moduleDir,
            OutputDirectory = outputDir,
            HeaderFiles = [Path.Combine(moduleDir, "include", "nlohmann", "json.hpp")],
            BuildCsPath = Path.Combine(moduleDir, "nlohmann_json.Build.cs"),
        };

        return (tempDir, input);
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