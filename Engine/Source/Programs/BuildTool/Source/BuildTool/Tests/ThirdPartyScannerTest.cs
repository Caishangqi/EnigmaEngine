using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Tests for ThirdParty module scanning and header-only INTERFACE target generation.
/// Validates:
///   [1]  ThirdPartyScanner discovers nlohmann_json module
///   [2]  Parsed module name matches "nlohmann_json"
///   [3]  Module is detected as header-only (no .cpp files)
///   [4]  PublicIncludePaths contains "include"
///   [5]  ModuleDirectory is set correctly
///   [6]  Scanner returns empty dict for non-existent directory
///   [7]  CMakeGenerator produces INTERFACE target for header-only module
///   [8]  No SHARED keyword in header-only target
///   [9]  No EXPORTS macro for header-only target
///   [10] No source file glob for header-only target
///   [11] INTERFACE include path resolves from ModuleDirectory
///   [12] Header-only module participates in dependency graph
///   [13] Dependent module gets correct link to INTERFACE target
///   [14] Build order: header-only before dependent
///   [15] Mixed graph: header-only + SHARED + executable all coexist
/// </summary>
public static class ThirdPartyScannerTest
{
    private static int _passed;
    private static int _failed;

    public static void Run()
    {
        _passed = 0;
        _failed = 0;

        Console.WriteLine("=== ThirdParty Scanner & Header-Only Module Tests ===");
        Console.WriteLine();

        TestScannerDiscoversModule();
        TestScannerNonExistentDir();
        TestHeaderOnlyInterfaceTarget();
        TestNoSharedNoExportsNoGlob();
        TestInterfaceIncludePath();
        TestDependencyGraphIntegration();
        TestMixedGraph();

        Console.WriteLine();
        Console.WriteLine($"=== {_passed}/{_passed + _failed} tests passed ===");

        if (_failed > 0)
            throw new Exception($"ThirdPartyScannerTest: {_failed} test(s) failed.");
    }

    /// <summary>Tests [1]-[5]: Scanner discovers nlohmann_json with correct properties.</summary>
    private static void TestScannerDiscoversModule()
    {
        Console.WriteLine("[Tests 1-5] ThirdPartyScanner discovers nlohmann_json");

        // Resolve ThirdParty root relative to build output
        var thirdPartyRoot = ResolveThirdPartyRoot();
        if (thirdPartyRoot is null)
        {
            Console.WriteLine("  SKIPPED (ThirdParty directory not found)");
            return;
        }

        var modules = ThirdPartyScanner.Scan(thirdPartyRoot);

        Check(modules.ContainsKey("nlohmann_json"),
            "[1]  ThirdPartyScanner discovers nlohmann_json module");

        var rules = modules["nlohmann_json"];

        Check(rules.ModuleName == "nlohmann_json",
            "[2]  Parsed module name = \"nlohmann_json\"");

        Check(rules.IsHeaderOnly,
            "[3]  Module detected as header-only (no .cpp files)");

        Check(rules.PublicIncludePaths.Contains("include"),
            "[4]  PublicIncludePaths contains \"include\"");

        Check(!string.IsNullOrEmpty(rules.ModuleDirectory)
           && rules.ModuleDirectory.Replace('\\', '/').Contains("nlohmann_json"),
            "[5]  ModuleDirectory set correctly");
    }

    /// <summary>Test [6]: Scanner returns empty for non-existent directory.</summary>
    private static void TestScannerNonExistentDir()
    {
        Console.WriteLine("[Test 6] Scanner handles non-existent directory");

        var modules = ThirdPartyScanner.Scan("/nonexistent/path/ThirdParty");

        Check(modules.Count == 0,
            "[6]  Scanner returns empty dict for non-existent directory");
    }

    /// <summary>Test [7]: CMakeGenerator produces INTERFACE target.</summary>
    private static void TestHeaderOnlyInterfaceTarget()
    {
        Console.WriteLine("[Test 7] CMakeGenerator produces INTERFACE target");

        var (content, _) = GenerateWithHeaderOnly();

        Check(content.Contains("add_library(nlohmann_json INTERFACE)"),
            "[7]  CMakeGenerator produces INTERFACE target for header-only module");
    }

    /// <summary>Tests [8]-[10]: No SHARED, no EXPORTS, no glob for header-only.</summary>
    private static void TestNoSharedNoExportsNoGlob()
    {
        Console.WriteLine("[Tests 8-10] No SHARED/EXPORTS/glob for header-only");

        var (content, _) = GenerateWithHeaderOnly();

        Check(!content.Contains("add_library(nlohmann_json SHARED"),
            "[8]  No SHARED keyword in header-only target");

        Check(!content.Contains("NLOHMANN_JSON_EXPORTS"),
            "[9]  No EXPORTS macro for header-only target");

        Check(!content.Contains("NLOHMANN_JSON_SOURCES"),
            "[10] No source file glob for header-only target");
    }

    /// <summary>Test [11]: Include path resolves from ModuleDirectory.</summary>
    private static void TestInterfaceIncludePath()
    {
        Console.WriteLine("[Test 11] INTERFACE include path from ModuleDirectory");

        var (content, _) = GenerateWithHeaderOnly();

        // Should contain the absolute path to include/ resolved from ModuleDirectory
        Check(content.Contains("target_include_directories(nlohmann_json")
           && content.Contains("INTERFACE")
           && content.Contains("/include\""),
            "[11] INTERFACE include path resolves from ModuleDirectory");
    }

    /// <summary>Tests [12]-[14]: Header-only module in dependency graph.</summary>
    private static void TestDependencyGraphIntegration()
    {
        Console.WriteLine("[Tests 12-14] Dependency graph integration");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["nlohmann_json"] = new()
            {
                ModuleName = "nlohmann_json",
                IsHeaderOnly = true,
                ModuleDirectory = "/engine/ThirdParty/nlohmann_json",
                PublicIncludePaths = { "include" },
            },
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core", "nlohmann_json" },
            },
        };

        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);

        Check(resolveResult.Success,
            "[12] Header-only module participates in dependency graph");

        var gen = new CMakeGenerator();
        var genResult = gen.Generate("TestProject", modules, resolveResult, "/project");

        Check(genResult.Success && genResult.Content.Contains("target_link_libraries(Engine")
           && genResult.Content.Contains("nlohmann_json"),
            "[13] Dependent module links to INTERFACE target");

        // Build order: nlohmann_json before Engine
        var jsonPos = genResult.Content.IndexOf("add_library(nlohmann_json INTERFACE", StringComparison.Ordinal);
        var enginePos = genResult.Content.IndexOf("add_library(Engine", StringComparison.Ordinal);
        Check(jsonPos >= 0 && enginePos >= 0 && jsonPos < enginePos,
            "[14] Build order: header-only before dependent");
    }

    /// <summary>Test [15]: Mixed graph with header-only + SHARED + executable.</summary>
    private static void TestMixedGraph()
    {
        Console.WriteLine("[Test 15] Mixed graph: header-only + SHARED + executable");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["nlohmann_json"] = new()
            {
                ModuleName = "nlohmann_json",
                IsHeaderOnly = true,
                ModuleDirectory = "/engine/ThirdParty/nlohmann_json",
                PublicIncludePaths = { "include" },
            },
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core", "nlohmann_json" },
            },
            ["MyGame"] = new()
            {
                ModuleName = "MyGame",
                PublicDependencyModuleNames = { "Engine" },
            },
        };

        var target = new TargetRules
        {
            TargetName = "MyGame",
            Type = TargetType.Game,
            ExtraModuleNames = { "MyGame" },
        };

        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);
        var gen = new CMakeGenerator();
        var genResult = gen.Generate("MyGame", modules, resolveResult, "/project", target);

        Check(genResult.Success
           && genResult.Content.Contains("add_library(nlohmann_json INTERFACE)")
           && genResult.Content.Contains("add_library(Core")
           && genResult.Content.Contains("add_library(Engine")
           && genResult.Content.Contains("add_executable(MyGame"),
            "[15] Mixed graph: header-only + SHARED + executable all coexist");
    }

    // ── Helpers ──

    private static (string Content, bool Success) GenerateWithHeaderOnly()
    {
        var modules = new Dictionary<string, ModuleRules>
        {
            ["nlohmann_json"] = new()
            {
                ModuleName = "nlohmann_json",
                IsHeaderOnly = true,
                ModuleDirectory = "/engine/ThirdParty/nlohmann_json",
                PublicIncludePaths = { "include" },
            },
        };

        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);
        var gen = new CMakeGenerator();
        var genResult = gen.Generate("TestProject", modules, resolveResult, "/project");

        return (genResult.Content, genResult.Success);
    }

    private static string? ResolveThirdPartyRoot()
    {
        var baseDir = AppContext.BaseDirectory;
        // Navigate from build output to Engine/Source/ThirdParty
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", "..", "..",
            "Engine", "Source", "ThirdParty"));

        if (Directory.Exists(candidate))
            return candidate;

        // Fallback: search upward for EnigmaEngine root
        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            var testDir = Path.Combine(dir.FullName, "Engine", "Source", "ThirdParty");
            if (Directory.Exists(testDir))
                return testDir;
            dir = dir.Parent;
        }

        return null;
    }

    private static void Check(bool cond, string name)
    {
        if (cond) { Console.WriteLine($"  [PASS] {name}"); ++_passed; }
        else      { Console.WriteLine($"  [FAIL] {name}"); ++_failed; }
    }
}
