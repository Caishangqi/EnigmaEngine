// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Build;

namespace BuildTool.Tests;

/// <summary>
/// Unit tests for ProjectScanner.
/// Runs against the real EnigmaArcade project files.
/// </summary>
public static class ProjectScannerTest
{
    public static void Run()
    {
        Console.WriteLine("=== ProjectScanner Tests ===");
        Console.WriteLine();

        TestScansAllModuleSources();
        TestResolvesProjectFile();
        TestFindsEngineRoot();
        TestReturnsResolveResult();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestScansAllModuleSources()
    {
        Console.WriteLine("[Test 1] AllModules contains all expected modules");
        var scan = ScanEnigmaArcade();

        string[] expected = ["Core", "Engine", "Launch", "EnigmaArcade", "ArcadeGameplay", "ArcadeFeature", "nlohmann_json"];
        foreach (var name in expected)
        {
            Assert(scan.AllModules.ContainsKey(name), $"Missing module: {name}");
        }

        Console.WriteLine($"  PASSED (found {scan.AllModules.Count} modules: {string.Join(", ", scan.AllModules.Keys.Order())})");
    }

    private static void TestResolvesProjectFile()
    {
        Console.WriteLine("[Test 2] ProjectName == \"EnigmaArcade\"");
        var scan = ScanEnigmaArcade();

        Assert(scan.ProjectName == "EnigmaArcade",
            $"Expected ProjectName 'EnigmaArcade', got '{scan.ProjectName}'");
        Assert(scan.EprojectPath.EndsWith("EnigmaArcade.eproject", StringComparison.OrdinalIgnoreCase),
            $"EprojectPath should end with EnigmaArcade.eproject, got '{scan.EprojectPath}'");

        Console.WriteLine($"  PASSED (project: {scan.ProjectName}, path: {scan.EprojectPath})");
    }

    private static void TestFindsEngineRoot()
    {
        Console.WriteLine("[Test 3] EngineRoot ends with \"Engine\"");
        var scan = ScanEnigmaArcade();

        string engineDirName = Path.GetFileName(scan.EngineRoot);
        Assert(engineDirName == "Engine",
            $"Expected EngineRoot to end with 'Engine', got '{engineDirName}' (full: {scan.EngineRoot})");
        Assert(Directory.Exists(Path.Combine(scan.EngineRoot, "Source", "Runtime")),
            "EngineRoot/Source/Runtime/ should exist");

        Console.WriteLine($"  PASSED (engine: {scan.EngineRoot})");
    }

    private static void TestReturnsResolveResult()
    {
        Console.WriteLine("[Test 4] ResolveResult.BuildOrder is non-empty");
        var scan = ScanEnigmaArcade();

        Assert(scan.ResolveResult.Success, "Dependency resolution should succeed");
        Assert(scan.ResolveResult.BuildOrder.Count > 0, "BuildOrder should be non-empty");

        // Core should appear before Engine in build order
        var order = scan.ResolveResult.BuildOrder.ToList();
        int coreIdx = order.IndexOf("Core");
        int engineIdx = order.IndexOf("Engine");
        Assert(coreIdx >= 0, "Core should be in BuildOrder");
        Assert(engineIdx >= 0, "Engine should be in BuildOrder");
        Assert(coreIdx < engineIdx, $"Core (idx={coreIdx}) should come before Engine (idx={engineIdx})");

        Console.WriteLine($"  PASSED (order: {string.Join(" → ", order)})");
    }

    private static ProjectScanner.ScanResult ScanEnigmaArcade()
    {
        var projectRoot = ResolveEnigmaArcadeRoot();
        string eprojectPath = Path.Combine(projectRoot, "EnigmaArcade.eproject");
        Assert(File.Exists(eprojectPath), $".eproject not found: {eprojectPath}");
        return ProjectScanner.Scan(eprojectPath);
    }

    private static string ResolveEnigmaArcadeRoot()
    {
        var baseDir = AppContext.BaseDirectory;
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", "..", "EnigmaArcade"));

        if (Directory.Exists(candidate))
            return candidate;

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
