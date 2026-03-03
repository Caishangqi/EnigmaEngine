// Copyright EnigmaEngine. All Rights Reserved.

using System.Diagnostics;
using System.Text;
using BuildTool.Commands;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// End-to-end integration test: scaffold a project with module + plugin,
/// then build with DebugGame / Development / Shipping and verify:
///   - Build succeeds
///   - EXE naming matches configuration
///   - DLL presence/absence matches configuration (modular vs monolithic)
///   - Manifest files match configuration
///   - EXE runs and produces correct engine/module/plugin output
///
/// Requires CMake + MSVC installed on the system.
/// </summary>
public static class ScaffoldingBuildIntegrationTest
{
    private static string _engineRoot = null!;
    private static string _projectName = null!;
    private static string _projectDir = null!;
    private static string _eprojectPath = null!;
    private static string _outputDir = null!;       // Project/Binaries/Win64 (game DLLs, Shipping EXE)
    private static string _engineOutputDir = null!;  // Engine/Binaries/Win64 (engine DLLs, Modular EXE)

    public static void Run()
    {
        Console.WriteLine("=== Scaffolding Build Integration Tests ===");
        Console.WriteLine();

        Setup();

        try
        {
            TestDebugGameBuild();
            TestDevelopmentBuild();
            TestShippingBuild();
        }
        finally
        {
            Cleanup();
        }

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    // ── Setup ────────────────────────────────────────────────────

    /// <summary>
    /// Scaffold a fresh project with an extra module and plugin,
    /// then wire the main module to depend on both.
    /// </summary>
    private static void Setup()
    {
        Console.WriteLine("[Setup] Finding engine root...");
        _engineRoot = FindEngineRoot();
        Console.WriteLine($"  Engine root: {_engineRoot}");

        _projectName = $"SE2E{Guid.NewGuid().ToString("N")[..6]}";
        var gamesDir = Path.Combine(_engineRoot, "..", "Games");
        _projectDir = Path.Combine(Path.GetFullPath(gamesDir), _projectName);
        _eprojectPath = Path.Combine(_projectDir, $"{_projectName}.eproject");
        _outputDir = Path.Combine(_projectDir, "Binaries", "Win64");
        _engineOutputDir = Path.Combine(_engineRoot, "Binaries", "Win64");

        // Clean stale directory if exists
        if (Directory.Exists(_projectDir))
            Directory.Delete(_projectDir, recursive: true);

        // 1. create-project
        Console.WriteLine($"[Setup] Creating project '{_projectName}'...");
        var createProjectResult = new CreateProjectCommand().Execute(new BuildOptions
        {
            ProjectPath = ".",
            ExtraArguments = new Dictionary<string, string>
            {
                ["name"] = _projectName,
                ["location"] = Path.GetFullPath(gamesDir),
            },
        });
        Assert(createProjectResult.Success,
            $"create-project failed: {createProjectResult.Message} | {createProjectResult.ErrorDetail}");
        Assert(File.Exists(_eprojectPath), $".eproject not found at {_eprojectPath}");

        // 2. create-module GameUtils
        Console.WriteLine("[Setup] Creating module 'GameUtils'...");
        var createModuleResult = RunCommand(new CreateModuleCommand(),
            new() { ["name"] = "GameUtils" });
        Assert(createModuleResult.Success,
            $"create-module failed: {createModuleResult.Message}");

        // 3. create-plugin TestFeature
        Console.WriteLine("[Setup] Creating plugin 'TestFeature'...");
        var createPluginResult = RunCommand(new CreatePluginCommand(),
            new() { ["name"] = "TestFeature", ["category"] = "Testing" });
        Assert(createPluginResult.Success,
            $"create-plugin failed: {createPluginResult.Message}");

        // 4. Wire main module to depend on GameUtils and TestFeature
        Console.WriteLine("[Setup] Configuring dependencies...");
        var buildCsPath = Path.Combine(
            _projectDir, "Source", _projectName, $"{_projectName}.Build.cs");
        var buildCs = File.ReadAllText(buildCsPath);
        buildCs = buildCs.Replace(
            "PublicDependencyModuleNames.Add(\"Engine\");",
            "PublicDependencyModuleNames.Add(\"Engine\");\n" +
            "\t\tPublicDependencyModuleNames.Add(\"GameUtils\");\n" +
            "\t\tPublicDependencyModuleNames.Add(\"TestFeature\");");
        File.WriteAllText(buildCsPath, buildCs);

        Console.WriteLine("[Setup] Project scaffolded successfully.");
        Console.WriteLine($"  Project: {_projectDir}");
        Console.WriteLine();
    }

    // ── Test: DebugGame ──────────────────────────────────────────

    private static void TestDebugGameBuild()
    {
        Console.WriteLine("[Test 1] DebugGame: build, verify layout, run");
        BuildVerifyAndRun(
            BuildConfiguration.DebugGame,
            expectedExeName: $"{_projectName}-Win64-DebugGame.exe",
            expectModular: true,
            dllSuffix: "-Win64-DebugGame");
        Console.WriteLine("  PASSED");
    }

    // ── Test: Development ────────────────────────────────────────

    private static void TestDevelopmentBuild()
    {
        Console.WriteLine("[Test 2] Development: build, verify layout, run");
        BuildVerifyAndRun(
            BuildConfiguration.Development,
            expectedExeName: $"{_projectName}.exe",
            expectModular: true,
            dllSuffix: "");
        Console.WriteLine("  PASSED");
    }

    // ── Test: Shipping ───────────────────────────────────────────

    private static void TestShippingBuild()
    {
        Console.WriteLine("[Test 3] Shipping: build, verify monolithic, run");
        BuildVerifyAndRun(
            BuildConfiguration.Shipping,
            expectedExeName: $"{_projectName}-Win64-Shipping.exe",
            expectModular: false,
            dllSuffix: "");
        Console.WriteLine("  PASSED");
    }

    // ── Core: Build → Verify → Run ─────────────────────────────

    /// <summary>
    /// Clean → Build → Verify binary layout → Run EXE → Verify output.
    /// </summary>
    private static void BuildVerifyAndRun(
        BuildConfiguration config,
        string expectedExeName,
        bool expectModular,
        string dllSuffix)
    {
        // 1. Clean previous build
        RunClean(config);

        // 2. Build
        Console.WriteLine($"  Building ({config})...");
        var buildResult = RunBuild(config);
        Assert(buildResult.Success,
            $"{config} build failed: {buildResult.Message} | {buildResult.ErrorDetail}");

        // Modular: EXE in Engine/Binaries/, Shipping: EXE in Project/Binaries/
        string exeDir = expectModular ? _engineOutputDir : _outputDir;
        Assert(Directory.Exists(exeDir), $"{exeDir} should exist");

        // 3. Verify EXE naming
        var exePath = Path.Combine(exeDir, expectedExeName);
        Assert(File.Exists(exePath),
            $"Expected EXE '{expectedExeName}' not found in {exeDir}. " +
            $"Files: [{string.Join(", ", Directory.GetFiles(exeDir).Select(Path.GetFileName))}]");
        Console.WriteLine($"  EXE: {expectedExeName} ✓");

        // 4. Verify DLL presence/absence
        if (expectModular)
        {
            // Engine DLLs in Engine/Binaries/
            var engineDlls = Directory.GetFiles(_engineOutputDir, "*.dll");
            Assert(engineDlls.Length > 0,
                $"{config} (modular) should have engine DLLs in Engine/Binaries/");

            // Game DLLs in Project/Binaries/
            Assert(Directory.Exists(_outputDir), "Project/Binaries/Win64/ should exist");
            var gameDlls = Directory.GetFiles(_outputDir, "*.dll");
            Assert(gameDlls.Length > 0,
                $"{config} (modular) should have game DLLs in Project/Binaries/");

            // Verify DLL naming convention
            if (!string.IsNullOrEmpty(dllSuffix))
            {
                var hasSuffixedDll = engineDlls.Concat(gameDlls).Any(f =>
                    Path.GetFileName(f).Contains(dllSuffix, StringComparison.OrdinalIgnoreCase));
                Assert(hasSuffixedDll,
                    $"DLLs should contain '{dllSuffix}' suffix for {config}");
            }
            Console.WriteLine($"  DLLs: {engineDlls.Length} engine + {gameDlls.Length} game (modular) ✓");

            // .modules manifest should exist in both locations
            var engineModules = Directory.GetFiles(_engineOutputDir, "*.modules");
            var gameModules = Directory.GetFiles(_outputDir, "*.modules");
            Assert(engineModules.Length > 0, $"{config} should have .modules manifest in Engine/Binaries/");
            Assert(gameModules.Length > 0, $"{config} should have .modules manifest in Project/Binaries/");
        }
        else
        {
            // Shipping: monolithic - no DLLs in project dir
            var dllFiles = Directory.GetFiles(_outputDir, "*.dll");
            Assert(dllFiles.Length == 0,
                $"Shipping (monolithic) should have NO DLLs. " +
                $"Found: [{string.Join(", ", dllFiles.Select(Path.GetFileName))}]");
            Console.WriteLine("  DLLs: 0 (monolithic) ✓");

            // .modules should NOT exist
            var modulesFiles = Directory.GetFiles(_outputDir, "*.modules");
            Assert(modulesFiles.Length == 0, "Shipping should have no .modules manifest");

            // .target should contain Monolithic
            var targetFiles = Directory.GetFiles(_outputDir, "*.target");
            Assert(targetFiles.Length > 0, "Should have .target manifest");
            var targetContent = File.ReadAllText(targetFiles[0]);
            Assert(targetContent.Contains("Monolithic", StringComparison.OrdinalIgnoreCase),
                ".target should contain 'Monolithic' for Shipping");
            Console.WriteLine("  Manifest: Monolithic ✓");
        }

        // 5. Run EXE and verify output
        Console.WriteLine($"  Running {expectedExeName}...");
        var output = RunExeAndCapture(exePath, expectModular ? _projectDir : null);

        Assert(output.Contains("[GuardedMain]"),
            $"Output should contain '[GuardedMain]'. Got:\n{Truncate(output, 500)}");
        Console.WriteLine("  Output: [GuardedMain] ✓");

        Assert(output.Contains("[GameUtils] StartupModule"),
            $"Output should contain '[GameUtils] StartupModule'. Got:\n{Truncate(output, 500)}");
        Console.WriteLine("  Output: [GameUtils] StartupModule ✓");

        Assert(output.Contains("[TestFeature] StartupModule"),
            $"Output should contain '[TestFeature] StartupModule'. Got:\n{Truncate(output, 500)}");
        Console.WriteLine("  Output: [TestFeature] StartupModule ✓");

        // 6. Clean after test
        RunClean(config);
    }

    // ── Helpers ────────────────────────────────────────────────

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }

    private static BuildResult RunCommand(ICommand command,
        Dictionary<string, string>? extra = null)
    {
        return command.Execute(new BuildOptions
        {
            ProjectPath = _projectDir,
            ExtraArguments = extra ?? [],
        });
    }

    private static BuildResult RunBuild(BuildConfiguration config)
    {
        return new BuildCommand().Execute(new BuildOptions
        {
            ProjectPath = _eprojectPath,
            Configuration = config,
            Platform = "Win64",
        });
    }

    private static void RunClean(BuildConfiguration config)
    {
        new CleanCommand().Execute(new BuildOptions
        {
            ProjectPath = _projectDir,
            Configuration = config,
        });
    }

    /// <summary>
    /// Launch the EXE, wait for engine startup, capture output.
    /// Kills the process after startup markers are found or timeout.
    /// </summary>
    private static string RunExeAndCapture(string exePath, string? projectDir = null)
    {
        var psi = new ProcessStartInfo
        {
            FileName = exePath,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
            WorkingDirectory = projectDir ?? Path.GetDirectoryName(exePath)!,
        };

        if (projectDir is not null)
        {
            psi.ArgumentList.Add($"--project-dir={projectDir}");
        }

        Process? proc = null;
        try
        {
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

            // Wait up to 15s for engine startup
            var sw = Stopwatch.StartNew();
            while (sw.ElapsedMilliseconds < 15_000 && !foundStartup && !proc.HasExited)
            {
                Thread.Sleep(100);
            }

            // Give async readers time to flush
            if (proc.HasExited && !foundStartup)
                Thread.Sleep(500);

            return output.ToString();
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

    /// <summary>
    /// Find the engine root by walking up from AppContext.BaseDirectory.
    /// Looks for Engine/Source/Runtime/ directory.
    /// </summary>
    private static string FindEngineRoot()
    {
        string current = AppContext.BaseDirectory;
        for (int i = 0; i < 10; i++)
        {
            string? parent = Path.GetDirectoryName(current);
            if (parent is null || parent == current) break;

            string candidate = Path.Combine(parent, "Engine");
            if (Directory.Exists(Path.Combine(candidate, "Source", "Runtime")))
                return candidate;

            current = parent;
        }

        throw new DirectoryNotFoundException(
            $"Engine root not found. Expected Engine/Source/Runtime/ above {AppContext.BaseDirectory}");
    }

    private static void Cleanup()
    {
        Console.WriteLine("[Cleanup] Removing scaffolded project...");
        try
        {
            if (Directory.Exists(_projectDir))
                Directory.Delete(_projectDir, recursive: true);

            // Clean engine binaries produced by this project
            if (Directory.Exists(_engineOutputDir))
            {
                foreach (var file in Directory.GetFiles(_engineOutputDir, $"{_projectName}*"))
                    File.Delete(file);
            }

            Console.WriteLine("  Done.");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"  Warning: cleanup failed: {ex.Message}");
        }
    }

    private static string Truncate(string s, int maxLen) =>
        s.Length <= maxLen ? s : s[..maxLen] + "...";
}
