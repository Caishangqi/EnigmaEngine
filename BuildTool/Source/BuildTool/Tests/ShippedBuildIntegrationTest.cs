// Copyright EnigmaEngine. All Rights Reserved.

using System.Diagnostics;
using System.Text;
using BuildTool.Build;
using BuildTool.Commands;
using BuildTool.Generators;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// End-to-end integration tests for the Shipping (monolithic) build pipeline
/// against the real EnigmaArcade project.
/// Requires CMake + MSVC installed on the system.
///
///   [1] Shipping build output: monolithic EXE, no DLLs, .target with Monolithic
///   [2] Shipping build runs and engine initializes
///   [3] ShippingPackager plan matches UE shipped directory structure
/// </summary>
public static class ShippedBuildIntegrationTest
{
    private static string _eprojectPath = null!;
    private static string _projectRoot = null!;
    private static string _projectName = null!;
    private static string _outputDir = null!;

    public static void Run()
    {
        Console.WriteLine("=== ShippedBuildIntegration Tests ===");
        Console.WriteLine();

        _eprojectPath = FindEnigmaArcadeProject();
        _projectRoot = Path.GetDirectoryName(Path.GetFullPath(_eprojectPath))!;
        _projectName = Path.GetFileNameWithoutExtension(_eprojectPath);
        _outputDir = Path.Combine(_projectRoot, "Binaries", "Win64");

        Console.WriteLine($"  Project: {_eprojectPath}");
        Console.WriteLine($"  Root:    {_projectRoot}");
        Console.WriteLine();

        // Clean slate
        RunClean();

        try
        {
            TestShippingBuildOutputMatchesUE();
            TestShippingBuildRuns();
            TestShippedPackageStructure();
        }
        finally
        {
            RunClean();
        }

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

// ── Helpers ──────────────────────────────────────────────

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }

    private static string FindEnigmaArcadeProject()
    {
        string current = Directory.GetCurrentDirectory();
        for (int i = 0; i < 6; i++)
        {
            string candidate = Path.Combine(current, "Games", "EnigmaArcade", "EnigmaArcade.eproject");
            if (File.Exists(candidate))
                return Path.GetFullPath(candidate);

            string? parent = Path.GetDirectoryName(current);
            if (parent is null || parent == current) break;
            current = parent;
        }

        throw new FileNotFoundException(
            "EnigmaArcade.eproject not found. Ensure the test runs from within the EnigmaEngine repo.");
    }

    private static BuildResult RunBuild(BuildConfiguration config)
    {
        var command = new BuildCommand();
        return command.Execute(new BuildOptions
        {
            ProjectPath = _eprojectPath,
            Configuration = config,
            Platform = "Win64",
        });
    }

    private static void RunClean()
    {
        var command = new CleanCommand();
        command.Execute(new BuildOptions
        {
            ProjectPath = _projectRoot,
            Configuration = BuildConfiguration.Shipping,
        });
    }

    // ── Test 1: Shipping build output matches UE ──────────

    private static void TestShippingBuildOutputMatchesUE()
    {
        Console.WriteLine("[Test 1] Shipping build output: monolithic EXE, no DLLs");

        var result = RunBuild(BuildConfiguration.Shipping);
        Assert(result.Success, $"Shipping build should succeed: {result.Message} | {result.ErrorDetail}");
        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist");

        // Monolithic EXE(s)
        var exeFiles = Directory.GetFiles(_outputDir, "*.exe");
        Assert(exeFiles.Length > 0, "Should have at least one .exe (monolithic)");

        // NO DLLs — all modules statically linked
        var dllFiles = Directory.GetFiles(_outputDir, "*.dll");
        Assert(dllFiles.Length == 0,
            $"Shipping build should have NO DLLs (monolithic), found: {string.Join(", ", dllFiles.Select(Path.GetFileName))}");

        // .target manifest with Monolithic link type
        var targetFiles = Directory.GetFiles(_outputDir, "*.target");
        Assert(targetFiles.Length > 0, "Should have .target manifest");
        string targetContent = File.ReadAllText(targetFiles[0]);
        Assert(targetContent.Contains("Monolithic", StringComparison.OrdinalIgnoreCase),
            $".target should contain 'Monolithic', got:\n{targetContent[..Math.Min(300, targetContent.Length)]}");

        // NO .modules file (not needed for monolithic)
        var modulesFiles = Directory.GetFiles(_outputDir, "*.modules");
        Assert(modulesFiles.Length == 0,
            $"Shipping build should have NO .modules file, found: {string.Join(", ", modulesFiles.Select(Path.GetFileName))}");

        // Flat layout — no subdirectories
        var subdirs = Directory.GetDirectories(_outputDir);
        Assert(subdirs.Length == 0,
            $"Binaries/Win64/ should be flat, found subdirs: {string.Join(", ", subdirs.Select(Path.GetFileName))}");

        Console.WriteLine($"  EXE: {exeFiles.Length}, DLL: 0 (monolithic)");
        Console.WriteLine($"  .target: Monolithic, .modules: none");
        Console.WriteLine("  PASSED");
    }

    // ── Test 2: Shipping build runs ─────────────────────────

    private static void TestShippingBuildRuns()
    {
        Console.WriteLine("[Test 2] Shipping monolithic EXE runs and engine initializes");

        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist from Test 1");
        var exeFiles = Directory.GetFiles(_outputDir, "*.exe");
        Assert(exeFiles.Length > 0, "No EXE found to run");

        // Pick monolithic EXE (named after project for Shipping config)
        string expectedExeName = $"{_projectName}-Win64-Shipping.exe";
        string exePath = exeFiles.FirstOrDefault(f =>
            Path.GetFileName(f).Equals(expectedExeName, StringComparison.OrdinalIgnoreCase))
            ?? exeFiles[0];

        Console.WriteLine($"  Launching: {Path.GetFileName(exePath)}");

        Process? proc = null;
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = exePath,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = _outputDir,
            };

            proc = Process.Start(psi)!;

            var output = new StringBuilder();
            bool foundStartup = false;

            proc.OutputDataReceived += (_, e) =>
            {
                if (e.Data is null) return;
                lock (output) { output.AppendLine(e.Data); }
                if (e.Data.Contains("[GuardedMain]"))
                    foundStartup = true;
            };
            proc.ErrorDataReceived += (_, e) =>
            {
                if (e.Data is not null)
                    lock (output) { output.AppendLine(e.Data); }
            };

            proc.BeginOutputReadLine();
            proc.BeginErrorReadLine();

            // Wait up to 10s for engine startup
            var sw = Stopwatch.StartNew();
            while (sw.ElapsedMilliseconds < 10_000 && !foundStartup && !proc.HasExited)
            {
                Thread.Sleep(100);
            }

            if (proc.HasExited && !foundStartup)
            {
                Thread.Sleep(500);
            }

            string captured = output.ToString();
            Assert(foundStartup,
                $"Output should contain '[GuardedMain]' (engine startup), " +
                $"exitCode={(!proc.HasExited ? "running" : proc.ExitCode.ToString())}, " +
                $"got:\n{captured[..Math.Min(500, captured.Length)]}");

            Console.WriteLine("  Engine startup confirmed (monolithic)");
            Console.WriteLine("  PASSED");
        }
        finally
        {
            if (proc is not null && !proc.HasExited)
            {
                try { proc.Kill(entireProcessTree: true); } catch { }
            }
            proc?.Dispose();
        }
    }

    // ── Test 3: Shipped package structure ──────────────────

    private static void TestShippedPackageStructure()
    {
        Console.WriteLine("[Test 3] ShippingPackager plan matches UE shipped layout");

        // Scan the real project
        var scan = ProjectScanner.Scan(_eprojectPath);

        // Generate packaging plan (pure function — no file I/O)
        var packager = new ShippingPackager();
        var engineModuleNames = new HashSet<string>(
            scan.EngineModules.Keys, StringComparer.Ordinal);

        var plan = packager.GeneratePlan(
            scan.ProjectName,
            scan.AllModules,
            scan.GameTarget,
            "Win64",
            scan.PluginScanResult,
            engineModuleNames);

        Assert(plan.Success, $"Package plan should succeed: {plan.Error}");
        Assert(plan.OutputDirectoryName == $"{scan.ProjectName}_Shipping",
            $"OutputDir should be '{scan.ProjectName}_Shipping', got '{plan.OutputDirectoryName}'");

        // Verify Launcher entry (root {ProjectName}.exe)
        var launchers = plan.Entries
            .Where(e => e.Category == ShippingPackager.EntryCategory.Launcher).ToList();
        Assert(launchers.Count > 0, "Plan should contain a Launcher entry");
        Assert(launchers[0].DestinationPath == $"{scan.ProjectName}.exe",
            $"Launcher should be '{scan.ProjectName}.exe', got '{launchers[0].DestinationPath}'");

        // Verify GameModule entry ({ProjectName}/Binaries/Win64/...)
        var gameModules = plan.Entries
            .Where(e => e.Category == ShippingPackager.EntryCategory.GameModule).ToList();
        Assert(gameModules.Count > 0, "Plan should contain a GameModule entry (monolithic EXE)");
        Assert(gameModules[0].DestinationPath.Contains($"{scan.ProjectName}/Binaries/Win64/"),
            $"GameModule should be under '{scan.ProjectName}/Binaries/Win64/', got '{gameModules[0].DestinationPath}'");

        // Verify Config entry
        var configs = plan.Entries
            .Where(e => e.Category == ShippingPackager.EntryCategory.Config).ToList();
        Assert(configs.Count > 0, "Plan should contain a Config entry");
        Assert(configs[0].DestinationPath.Contains("Engine/Config/"),
            $"Config should be under 'Engine/Config/', got '{configs[0].DestinationPath}'");

        // Verify Manifest entries
        var manifests = plan.Entries
            .Where(e => e.Category == ShippingPackager.EntryCategory.Manifest).ToList();
        Assert(manifests.Count > 0, "Plan should contain Manifest entries");

        Console.WriteLine($"  Plan: {plan.Entries.Count} entries");
        Console.WriteLine($"  Launcher: {launchers.Count}, GameModule: {gameModules.Count}");
        Console.WriteLine($"  Config: {configs.Count}, Manifest: {manifests.Count}");
        Console.WriteLine("  PASSED");
    }
}
