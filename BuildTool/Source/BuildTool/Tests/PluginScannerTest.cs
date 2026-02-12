using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Tests for PluginScanner: discovery, enabled/disabled filtering,
/// module parsing, and dependency graph integration.
/// Validates:
///   [1]  Scanner discovers InventorySystem plugin
///   [2]  Scanner discovers DebugTools plugin
///   [3]  Enabled plugin (InventorySystem) is in EnabledPlugins
///   [4]  Disabled plugin (DebugTools) is in DisabledPlugins
///   [5]  Enabled plugin modules parsed: InventoryCore
///   [6]  Enabled plugin modules parsed: InventoryUI
///   [7]  Disabled plugin modules NOT in result
///   [8]  InventoryCore has correct PublicDependencyModuleNames
///   [9]  InventoryUI has correct PrivateDependencyModuleNames
///   [10] ModuleDirectory set for InventoryCore
///   [11] Modules are not header-only (have .cpp files)
///   [12] Scanner returns empty for non-existent directory
///   [13] Scanner returns empty when no plugins enabled
///   [14] Plugin modules participate in dependency graph
///   [15] Build order respects plugin module dependencies
///   [16] CMake output contains plugin module targets
///   [17] Mixed graph: engine + plugin modules coexist
///   [18] Plugin with multiple modules: both appear in result
/// </summary>
public static class PluginScannerTest
{
    private static int _passed;
    private static int _failed;

    public static void Run()
    {
        _passed = 0;
        _failed = 0;

        Console.WriteLine("=== PluginScanner Tests ===");
        Console.WriteLine();

        var testDir = ResolveTestPluginsDir();

        TestDiscoveryAndFiltering(testDir);
        TestModuleParsing(testDir);
        TestNonExistentDir();
        TestNoPluginsEnabled(testDir);
        TestDependencyGraphIntegration();
        TestMixedGraph();

        Console.WriteLine();
        Console.WriteLine($"=== {_passed}/{_passed + _failed} tests passed ===");

        if (_failed > 0)
            throw new Exception($"PluginScannerTest: {_failed} test(s) failed.");
    }

    /// <summary>Tests [1]-[4]: Discovery and enabled/disabled filtering.</summary>
    private static void TestDiscoveryAndFiltering(string testDir)
    {
        Console.WriteLine("[Tests 1-4] Discovery and enabled/disabled filtering");

        // InventorySystem enabled, DebugTools disabled
        var projectPlugins = new List<PluginReference>
        {
            new() { Name = "InventorySystem", Enabled = true },
            new() { Name = "DebugTools", Enabled = false },
        };

        var result = PluginScanner.Scan(testDir, projectPlugins);

        Check(result.EnabledPlugins.Count + result.DisabledPlugins.Count >= 2,
            "[1]  Scanner discovers InventorySystem plugin");

        Check(result.DisabledPlugins.Contains("DebugTools"),
            "[2]  Scanner discovers DebugTools plugin");

        Check(result.EnabledPlugins.ContainsKey("InventorySystem"),
            "[3]  Enabled plugin (InventorySystem) is in EnabledPlugins");

        Check(result.DisabledPlugins.Contains("DebugTools")
           && !result.EnabledPlugins.ContainsKey("DebugTools"),
            "[4]  Disabled plugin (DebugTools) is in DisabledPlugins");
    }

    /// <summary>Tests [5]-[11]: Module parsing from enabled plugins.</summary>
    private static void TestModuleParsing(string testDir)
    {
        Console.WriteLine("[Tests 5-11] Module parsing from enabled plugins");

        var projectPlugins = new List<PluginReference>
        {
            new() { Name = "InventorySystem", Enabled = true },
            new() { Name = "DebugTools", Enabled = false },
        };

        var result = PluginScanner.Scan(testDir, projectPlugins);

        Check(result.Modules.ContainsKey("InventoryCore"),
            "[5]  Enabled plugin modules parsed: InventoryCore");

        Check(result.Modules.ContainsKey("InventoryUI"),
            "[6]  Enabled plugin modules parsed: InventoryUI");

        Check(!result.Modules.ContainsKey("DebugTools"),
            "[7]  Disabled plugin modules NOT in result");

        Check(result.Modules["InventoryCore"].PublicDependencyModuleNames.Contains("Core"),
            "[8]  InventoryCore has correct PublicDependencyModuleNames");

        Check(result.Modules["InventoryUI"].PrivateDependencyModuleNames.Contains("InventoryCore"),
            "[9]  InventoryUI has correct PrivateDependencyModuleNames");

        Check(!string.IsNullOrEmpty(result.Modules["InventoryCore"].ModuleDirectory)
           && result.Modules["InventoryCore"].ModuleDirectory.Replace('\\', '/').Contains("InventoryCore"),
            "[10] ModuleDirectory set for InventoryCore");

        Check(!result.Modules["InventoryCore"].IsHeaderOnly
           && !result.Modules["InventoryUI"].IsHeaderOnly,
            "[11] Modules are not header-only (have .cpp files)");
    }

    /// <summary>Test [12]: Non-existent directory returns empty.</summary>
    private static void TestNonExistentDir()
    {
        Console.WriteLine("[Test 12] Non-existent directory");

        var result = PluginScanner.Scan("/nonexistent/path/Plugins", []);

        Check(result.Modules.Count == 0
           && result.EnabledPlugins.Count == 0
           && result.DisabledPlugins.Count == 0,
            "[12] Scanner returns empty for non-existent directory");
    }

    /// <summary>Test [13]: No plugins enabled returns empty modules.</summary>
    private static void TestNoPluginsEnabled(string testDir)
    {
        Console.WriteLine("[Test 13] No plugins enabled");

        // All plugins disabled or unlisted
        var projectPlugins = new List<PluginReference>
        {
            new() { Name = "InventorySystem", Enabled = false },
            new() { Name = "DebugTools", Enabled = false },
        };

        var result = PluginScanner.Scan(testDir, projectPlugins);

        Check(result.Modules.Count == 0,
            "[13] Scanner returns empty when no plugins enabled");
    }

    /// <summary>Tests [14]-[16]: Plugin modules in dependency graph and CMake.</summary>
    private static void TestDependencyGraphIntegration()
    {
        Console.WriteLine("[Tests 14-16] Dependency graph integration");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["InventoryCore"] = new()
            {
                ModuleName = "InventoryCore",
                PublicDependencyModuleNames = { "Core" },
            },
            ["InventoryUI"] = new()
            {
                ModuleName = "InventoryUI",
                PublicDependencyModuleNames = { "Core" },
                PrivateDependencyModuleNames = { "InventoryCore" },
            },
        };

        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);

        Check(resolveResult.Success,
            "[14] Plugin modules participate in dependency graph");

        // InventoryCore before InventoryUI in build order
        var order = resolveResult.BuildOrder.ToList();
        var coreIdx = order.IndexOf("InventoryCore");
        var uiIdx = order.IndexOf("InventoryUI");
        Check(coreIdx >= 0 && uiIdx >= 0 && coreIdx < uiIdx,
            "[15] Build order respects plugin module dependencies");

        var gen = new CMakeGenerator();
        var genResult = gen.Generate("TestProject", modules, resolveResult, "/project");

        Check(genResult.Success
           && genResult.Content.Contains("add_library(InventoryCore")
           && genResult.Content.Contains("add_library(InventoryUI"),
            "[16] CMake output contains plugin module targets");
    }

    /// <summary>Tests [17]-[18]: Mixed graph and multi-module plugin.</summary>
    private static void TestMixedGraph()
    {
        Console.WriteLine("[Tests 17-18] Mixed graph and multi-module plugin");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core" },
            },
            ["InventoryCore"] = new()
            {
                ModuleName = "InventoryCore",
                PublicDependencyModuleNames = { "Core" },
            },
            ["InventoryUI"] = new()
            {
                ModuleName = "InventoryUI",
                PublicDependencyModuleNames = { "Core" },
                PrivateDependencyModuleNames = { "InventoryCore" },
            },
            ["MyGame"] = new()
            {
                ModuleName = "MyGame",
                PublicDependencyModuleNames = { "Engine", "InventoryCore", "InventoryUI" },
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
           && genResult.Content.Contains("add_library(Core")
           && genResult.Content.Contains("add_library(Engine")
           && genResult.Content.Contains("add_library(InventoryCore")
           && genResult.Content.Contains("add_library(InventoryUI")
           && genResult.Content.Contains("add_executable(MyGame"),
            "[17] Mixed graph: engine + plugin modules coexist");

        Check(genResult.Content.Contains("add_library(InventoryCore")
           && genResult.Content.Contains("add_library(InventoryUI"),
            "[18] Plugin with multiple modules: both appear in result");
    }

    // -- Helpers --

    private static string ResolveTestPluginsDir()
    {
        var baseDir = AppContext.BaseDirectory;
        // Navigate from build output to Tests/TestPlugins
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "Tests", "TestPlugins"));

        if (Directory.Exists(candidate))
            return candidate;

        // Fallback: search upward
        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            var testDir = Path.Combine(dir.FullName, "Tests", "TestPlugins");
            if (Directory.Exists(testDir))
                return testDir;
            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("TestPlugins directory not found.");
    }

    private static void Check(bool cond, string name)
    {
        if (cond) { Console.WriteLine($"  [PASS] {name}"); ++_passed; }
        else      { Console.WriteLine($"  [FAIL] {name}"); ++_failed; }
    }
}
