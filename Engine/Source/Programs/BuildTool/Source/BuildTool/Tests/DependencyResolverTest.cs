using System.Diagnostics;
using BuildTool.Analysis;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for DependencyResolver and CycleDetector.
/// </summary>
public static class DependencyResolverTest
{
    public static void Run()
    {
        Console.WriteLine("=== DependencyResolver Smoke Tests ===");
        Console.WriteLine();

        TestLinearChain();
        TestDiamondDependency();
        TestCycleDetection();
        TestSelfCycle();
        TestNoModules();
        TestPerformance();
        TestReachableSetBasic();
        TestReachableSetMultipleRoots();
        TestReachableSetEmpty();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    /// <summary>
    /// Linear chain: Launch -> Engine -> Core
    /// Expected build order: Core, Engine, Launch
    /// </summary>
    private static void TestLinearChain()
    {
        Console.WriteLine("[Test 1] Linear chain: Launch -> Engine -> Core");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core" },
            },
            ["Launch"] = new()
            {
                ModuleName = "Launch",
                PublicDependencyModuleNames = { "Engine" },
                PrivateDependencyModuleNames = { "Core" },
            },
        };

        var result = new DependencyResolver().Resolve(modules);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        Assert(result.BuildOrder.Count == 3, $"BuildOrder.Count: expected 3, got {result.BuildOrder.Count}");

        // Core must come before Engine, Engine before Launch
        var coreIdx = result.BuildOrder.IndexOf("Core");
        var engineIdx = result.BuildOrder.IndexOf("Engine");
        var launchIdx = result.BuildOrder.IndexOf("Launch");
        Assert(coreIdx < engineIdx, $"Core ({coreIdx}) should come before Engine ({engineIdx})");
        Assert(engineIdx < launchIdx, $"Engine ({engineIdx}) should come before Launch ({launchIdx})");

        Console.WriteLine($"  PASSED (order: {string.Join(", ", result.BuildOrder)})");
    }

    /// <summary>
    /// Diamond: D -> B, D -> C, B -> A, C -> A
    /// Expected: A before B and C, B and C before D
    /// </summary>
    private static void TestDiamondDependency()
    {
        Console.WriteLine("[Test 2] Diamond dependency: D -> B,C -> A");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["A"] = new() { ModuleName = "A" },
            ["B"] = new() { ModuleName = "B", PublicDependencyModuleNames = { "A" } },
            ["C"] = new() { ModuleName = "C", PrivateDependencyModuleNames = { "A" } },
            ["D"] = new() { ModuleName = "D", PublicDependencyModuleNames = { "B", "C" } },
        };

        var result = new DependencyResolver().Resolve(modules);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        Assert(result.BuildOrder.Count == 4, $"BuildOrder.Count: expected 4, got {result.BuildOrder.Count}");

        var aIdx = result.BuildOrder.IndexOf("A");
        var bIdx = result.BuildOrder.IndexOf("B");
        var cIdx = result.BuildOrder.IndexOf("C");
        var dIdx = result.BuildOrder.IndexOf("D");
        Assert(aIdx < bIdx, "A should come before B");
        Assert(aIdx < cIdx, "A should come before C");
        Assert(bIdx < dIdx, "B should come before D");
        Assert(cIdx < dIdx, "C should come before D");

        Console.WriteLine($"  PASSED (order: {string.Join(", ", result.BuildOrder)})");
    }

    /// <summary>
    /// Cycle: A -> B -> C -> A
    /// </summary>
    private static void TestCycleDetection()
    {
        Console.WriteLine("[Test 3] Cycle detection: A -> B -> C -> A");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["A"] = new() { ModuleName = "A", PublicDependencyModuleNames = { "B" } },
            ["B"] = new() { ModuleName = "B", PublicDependencyModuleNames = { "C" } },
            ["C"] = new() { ModuleName = "C", PublicDependencyModuleNames = { "A" } },
        };

        var result = new DependencyResolver().Resolve(modules);

        Assert(!result.Success, "Expected failure due to cycle");
        Assert(result.CycleInfo is not null, "CycleInfo should be set");
        Assert(result.CycleInfo!.HasCycle, "HasCycle should be true");
        Assert(result.CycleInfo.CyclePath.Count >= 3, $"CyclePath should have >=3 nodes, got {result.CycleInfo.CyclePath.Count}");
        Assert(result.CycleInfo.CyclePath[0] == result.CycleInfo.CyclePath[^1], "Cycle path should start and end with same node");

        Console.WriteLine($"  PASSED ({result.CycleInfo.Description})");
    }

    /// <summary>
    /// Self-cycle: A -> A
    /// </summary>
    private static void TestSelfCycle()
    {
        Console.WriteLine("[Test 4] Self-cycle: A -> A");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["A"] = new() { ModuleName = "A", PublicDependencyModuleNames = { "A" } },
        };

        var result = new DependencyResolver().Resolve(modules);

        Assert(!result.Success, "Expected failure due to self-cycle");
        Assert(result.CycleInfo!.HasCycle, "HasCycle should be true");

        Console.WriteLine($"  PASSED ({result.CycleInfo.Description})");
    }

    /// <summary>
    /// Empty graph: no modules.
    /// </summary>
    private static void TestNoModules()
    {
        Console.WriteLine("[Test 5] Empty graph (no modules)");

        var modules = new Dictionary<string, ModuleRules>();
        var result = new DependencyResolver().Resolve(modules);

        Assert(result.Success, "Expected success for empty graph");
        Assert(result.BuildOrder.Count == 0, $"BuildOrder.Count: expected 0, got {result.BuildOrder.Count}");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Performance test: 1000+ modules in a chain, must complete within 5 seconds.
    /// </summary>
    private static void TestPerformance()
    {
        Console.WriteLine("[Test 6] Performance: 2000 modules chain");

        const int moduleCount = 2000;
        var modules = new Dictionary<string, ModuleRules>(moduleCount);

        for (int i = 0; i < moduleCount; i++)
        {
            var name = $"Module_{i:D4}";
            var rules = new ModuleRules { ModuleName = name };
            if (i > 0)
            {
                rules.PublicDependencyModuleNames.Add($"Module_{i - 1:D4}");
            }
            modules[name] = rules;
        }

        var sw = Stopwatch.StartNew();
        var result = new DependencyResolver().Resolve(modules);
        sw.Stop();

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        Assert(result.BuildOrder.Count == moduleCount, $"BuildOrder.Count: expected {moduleCount}, got {result.BuildOrder.Count}");
        Assert(sw.ElapsedMilliseconds < 5000, $"Performance: took {sw.ElapsedMilliseconds}ms, limit is 5000ms");

        // Verify order: Module_0000 should be first (no deps), Module_1999 last
        Assert(result.BuildOrder[0] == "Module_0000", $"First module should be Module_0000, got {result.BuildOrder[0]}");
        Assert(result.BuildOrder[^1] == $"Module_{moduleCount - 1:D4}", $"Last module should be Module_{moduleCount - 1:D4}");

        Console.WriteLine($"  PASSED ({moduleCount} modules resolved in {sw.ElapsedMilliseconds}ms)");
    }

    /// <summary>
    /// Reachable set: Launch -> Engine -> Core, googletest isolated.
    /// From root {Launch}, reachable = {Launch, Engine, Core}.
    /// </summary>
    private static void TestReachableSetBasic()
    {
        Console.WriteLine("[Test 7] ComputeReachableSet: basic reachability");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core" },
            },
            ["Launch"] = new()
            {
                ModuleName = "Launch",
                PublicDependencyModuleNames = { "Engine" },
            },
            ["googletest"] = new() { ModuleName = "googletest" },
        };

        var result = new DependencyResolver().Resolve(modules);
        Assert(result.Success, $"Expected success, got error: {result.Error}");

        var reachable = DependencyResolver.ComputeReachableSet(
            new[] { "Launch" }, result.AdjacencyList);

        Assert(reachable.Contains("Launch"), "Launch should be reachable");
        Assert(reachable.Contains("Engine"), "Engine should be reachable");
        Assert(reachable.Contains("Core"), "Core should be reachable");
        Assert(!reachable.Contains("googletest"), "googletest should NOT be reachable");
        Assert(reachable.Count == 3, $"Reachable count: expected 3, got {reachable.Count}");

        Console.WriteLine($"  PASSED (reachable: {string.Join(", ", reachable)})");
    }

    /// <summary>
    /// Multiple roots with shared dependencies.
    /// GameA -> Engine -> Core, GameB -> Core, TestLib isolated.
    /// From roots {GameA, GameB}, reachable = {GameA, GameB, Engine, Core}.
    /// </summary>
    private static void TestReachableSetMultipleRoots()
    {
        Console.WriteLine("[Test 8] ComputeReachableSet: multiple roots, shared deps");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core" },
            },
            ["GameA"] = new()
            {
                ModuleName = "GameA",
                PublicDependencyModuleNames = { "Engine" },
            },
            ["GameB"] = new()
            {
                ModuleName = "GameB",
                PublicDependencyModuleNames = { "Core" },
            },
            ["TestLib"] = new()
            {
                ModuleName = "TestLib",
                PublicDependencyModuleNames = { "Core" },
            },
        };

        var result = new DependencyResolver().Resolve(modules);
        Assert(result.Success, $"Expected success, got error: {result.Error}");

        var reachable = DependencyResolver.ComputeReachableSet(
            new[] { "GameA", "GameB" }, result.AdjacencyList);

        Assert(reachable.Contains("GameA"), "GameA should be reachable");
        Assert(reachable.Contains("GameB"), "GameB should be reachable");
        Assert(reachable.Contains("Engine"), "Engine should be reachable (via GameA)");
        Assert(reachable.Contains("Core"), "Core should be reachable (via both)");
        Assert(!reachable.Contains("TestLib"), "TestLib should NOT be reachable");
        Assert(reachable.Count == 4, $"Reachable count: expected 4, got {reachable.Count}");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Empty roots: reachable set should be empty.
    /// </summary>
    private static void TestReachableSetEmpty()
    {
        Console.WriteLine("[Test 9] ComputeReachableSet: empty roots");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };

        var result = new DependencyResolver().Resolve(modules);
        Assert(result.Success, $"Expected success, got error: {result.Error}");

        var reachable = DependencyResolver.ComputeReachableSet(
            Array.Empty<string>(), result.AdjacencyList);

        Assert(reachable.Count == 0, $"Reachable count: expected 0, got {reachable.Count}");

        Console.WriteLine("  PASSED");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception($"Assertion failed: {message}");
        }
    }

    private static int IndexOf(this IReadOnlyList<string> list, string item)
    {
        for (int i = 0; i < list.Count; i++)
        {
            if (list[i] == item) return i;
        }
        return -1;
    }
}
