using BuildTool.Analysis;
using BuildTool.Models;
using BuildTool.Scaffolding;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Unit tests for <see cref="DependencyChecker"/>.
/// </summary>
public static class DependencyCheckerTest
{
    public static void Run()
    {
        Console.WriteLine("=== DependencyChecker Tests ===");
        Console.WriteLine();

        TestNoDependents();
        TestHasDependents();
        TestPluginDependents();
        TestPluginNoDependents();
        TestSelfDependencyIgnored();
        TestCaseInsensitive();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestNoDependents()
    {
        Console.WriteLine("[Test 1] Module with no dependents returns Safe");

        // A→Core, B→Core, Orphan→(nothing)
        var resolve = MakeResolveResult(new Dictionary<string, List<string>>
        {
            ["A"] = ["Core"],
            ["B"] = ["Core"],
            ["Orphan"] = [],
        });

        var result = DependencyChecker.FindDependents("Orphan", resolve);
        Assert(result.IsSafeToRemove, "Orphan should have no dependents");
        Assert(result.Dependents.Count == 0, $"Expected 0 dependents, got {result.Dependents.Count}");

        Console.WriteLine("  PASSED");
    }

    private static void TestHasDependents()
    {
        Console.WriteLine("[Test 2] Module depended on by others returns Blocked");

        // A→Core, B→Core, C→A
        var resolve = MakeResolveResult(new Dictionary<string, List<string>>
        {
            ["A"] = ["Core"],
            ["B"] = ["Core"],
            ["C"] = ["A"],
            ["Core"] = [],
        });

        var result = DependencyChecker.FindDependents("Core", resolve);
        Assert(!result.IsSafeToRemove, "Core should have dependents");
        Assert(result.Dependents.Count == 2, $"Expected 2 dependents, got {result.Dependents.Count}");
        Assert(result.Dependents.Contains("A"), "A should depend on Core");
        Assert(result.Dependents.Contains("B"), "B should depend on Core");

        Console.WriteLine("  PASSED");
    }

    private static void TestPluginDependents()
    {
        Console.WriteLine("[Test 3] Plugin module depended on by game module returns Blocked");

        // GameModule→PluginMod, PluginMod→Core
        var resolve = MakeResolveResult(new Dictionary<string, List<string>>
        {
            ["GameModule"] = ["PluginMod"],
            ["PluginMod"] = ["Core"],
            ["Core"] = [],
        });

        var pluginScan = new PluginScanner.ScanResult
        {
            EnabledPlugins = new Dictionary<string, PluginDescriptor>(StringComparer.Ordinal)
            {
                ["MyPlugin"] = new()
                {
                    Modules = [new ModuleDescriptor { Name = "PluginMod" }],
                },
            },
        };

        var result = DependencyChecker.FindPluginDependents("MyPlugin", pluginScan, resolve);
        Assert(!result.IsSafeToRemove, "Plugin should have external dependents");
        Assert(result.Dependents.Contains("GameModule"), "GameModule should depend on plugin");
        Assert(!result.Dependents.Contains("PluginMod"), "Plugin's own module should not be in dependents");

        Console.WriteLine("  PASSED");
    }

    private static void TestPluginNoDependents()
    {
        Console.WriteLine("[Test 4] Plugin with no external dependents returns Safe");

        // PluginMod→Core, GameModule→Core (no dep on PluginMod)
        var resolve = MakeResolveResult(new Dictionary<string, List<string>>
        {
            ["GameModule"] = ["Core"],
            ["PluginMod"] = ["Core"],
            ["Core"] = [],
        });

        var pluginScan = new PluginScanner.ScanResult
        {
            EnabledPlugins = new Dictionary<string, PluginDescriptor>(StringComparer.Ordinal)
            {
                ["MyPlugin"] = new()
                {
                    Modules = [new ModuleDescriptor { Name = "PluginMod" }],
                },
            },
        };

        var result = DependencyChecker.FindPluginDependents("MyPlugin", pluginScan, resolve);
        Assert(result.IsSafeToRemove, "Plugin should have no external dependents");

        Console.WriteLine("  PASSED");
    }

    private static void TestSelfDependencyIgnored()
    {
        Console.WriteLine("[Test 5] Self-dependency is not counted");

        // Core depends on itself (edge case) + A depends on Core
        var resolve = MakeResolveResult(new Dictionary<string, List<string>>
        {
            ["Core"] = ["Core"],
            ["A"] = ["Core"],
        });

        var result = DependencyChecker.FindDependents("Core", resolve);
        Assert(result.Dependents.Count == 1, $"Expected 1 dependent (not self), got {result.Dependents.Count}");
        Assert(result.Dependents.Contains("A"), "A should be the only dependent");

        Console.WriteLine("  PASSED");
    }

    private static void TestCaseInsensitive()
    {
        Console.WriteLine("[Test 6] Case-insensitive dependency matching");

        // A depends on "core" (lowercase), checking for "Core" (mixed case)
        var resolve = MakeResolveResult(new Dictionary<string, List<string>>
        {
            ["A"] = ["core"],
            ["Core"] = [],
        });

        var result = DependencyChecker.FindDependents("Core", resolve);
        Assert(!result.IsSafeToRemove, "Should find A as dependent (case-insensitive)");
        Assert(result.Dependents.Contains("A"), "A should depend on Core (case-insensitive)");

        Console.WriteLine("  PASSED");
    }

    // --- Helpers ---

    private static DependencyResolver.ResolveResult MakeResolveResult(
        Dictionary<string, List<string>> adjacencyList) =>
        new()
        {
            Success = true,
            BuildOrder = adjacencyList.Keys.ToList(),
            AdjacencyList = adjacencyList,
        };

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
