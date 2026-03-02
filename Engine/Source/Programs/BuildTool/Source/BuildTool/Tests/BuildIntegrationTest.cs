// Copyright EnigmaEngine. All Rights Reserved.

using System.Diagnostics;
using System.Text;
using System.Xml.Linq;
using BuildTool.Build;
using BuildTool.Commands;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// End-to-end integration tests for the build pipeline against the real EnigmaArcade project.
/// Requires CMake + MSVC installed on the system.
///
///   [1] Development build output matches UE flat layout
///   [2] Development build runs and produces game tick output
///   [3] IDE build via NMake command extracted from Launch.vcxproj
///   [4] NMakeOutput path alignment with actual built EXE
///   [5] Incremental build completes under 10 seconds
///   [6] Clean and rebuild restores output
/// </summary>
public static class BuildIntegrationTest
{
    private static string _eprojectPath = null!;
    private static string _projectRoot = null!;
    private static string _projectName = null!;
    private static string _outputDir = null!;

    public static void Run()
    {
        Console.WriteLine("=== BuildIntegration Tests ===");
        Console.WriteLine();

        _eprojectPath = FindEnigmaArcadeProject();
        _projectRoot = Path.GetDirectoryName(Path.GetFullPath(_eprojectPath))!;
        _projectName = Path.GetFileNameWithoutExtension(_eprojectPath);
        _outputDir = Path.Combine(_projectRoot, "Binaries", "Win64");

        Console.WriteLine($"  Project: {_eprojectPath}");
        Console.WriteLine($"  Root:    {_projectRoot}");
        Console.WriteLine();

        // Clean slate before all tests
        RunClean();

        try
        {
            TestDevelopmentBuildOutputMatchesUE();
            TestDevelopmentBuildRuns();
            TestIDEBuildViaNMake();
            TestNMakeOutputPathAlignment();
            TestIncrementalBuild();
            TestCleanAndRebuild();
        }
        finally
        {
            // Final cleanup - restore clean state
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

    /// <summary>
    /// Locate EnigmaArcade.eproject by searching upward from the working directory.
    /// </summary>
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

    /// <summary>Run BuildCommand with the given configuration.</summary>
    private static BuildResult RunBuild(BuildConfiguration config = BuildConfiguration.Development)
    {
        var command = new BuildCommand();
        return command.Execute(new BuildOptions
        {
            ProjectPath = _eprojectPath,
            Configuration = config,
            Platform = "Win64",
        });
    }

    /// <summary>Run CleanCommand.</summary>
    private static void RunClean()
    {
        var command = new CleanCommand();
        command.Execute(new BuildOptions
        {
            ProjectPath = _projectRoot,
            Configuration = BuildConfiguration.Development,
        });
    }

    /// <summary>Run GenerateProjectFilesCommand to produce .vcxproj files.</summary>
    private static BuildResult RunGenerateProjectFiles()
    {
        var command = new GenerateProjectFilesCommand();
        return command.Execute(new BuildOptions
        {
            ProjectPath = _eprojectPath,
        });
    }

    /// <summary>
    /// Find Launch.vcxproj in Intermediate/ProjectFiles/.
    /// </summary>
    private static string FindLaunchVcxproj()
    {
        string intermediateDir = Path.Combine(_projectRoot, "Intermediate", "ProjectFiles");
        string vcxprojPath = Path.Combine(intermediateDir, "Launch.vcxproj");
        if (File.Exists(vcxprojPath))
            return vcxprojPath;

        // Fallback: search recursively
        var files = Directory.GetFiles(intermediateDir, "Launch.vcxproj", SearchOption.AllDirectories);
        Assert(files.Length > 0, "Launch.vcxproj not found in Intermediate/ProjectFiles/");
        return files[0];
    }

    /// <summary>
    /// Parse an NMake property from a .vcxproj for the Development configuration.
    /// </summary>
    private static string ParseNMakeProperty(string vcxprojPath, string propertyName)
    {
        var doc = XDocument.Load(vcxprojPath);
        XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";

        // Look for Development|x64 condition
        foreach (var pg in doc.Descendants(ns + "PropertyGroup"))
        {
            var condition = pg.Attribute("Condition")?.Value ?? "";
            if (!condition.Contains("Development", StringComparison.OrdinalIgnoreCase))
                continue;

            var element = pg.Element(ns + propertyName);
            if (element is not null)
                return element.Value;
        }

        throw new InvalidOperationException(
            $"{propertyName} not found in {Path.GetFileName(vcxprojPath)} for Development config");
    }

    /// <summary>
    /// Run a shell command and return (exitCode, stdout).
    /// </summary>
    private static (int ExitCode, string Output) RunShellCommand(
        string command, string workingDir, int timeoutMs = 300_000)
    {
        var psi = new ProcessStartInfo
        {
            FileName = "cmd.exe",
            Arguments = $"/S /c \"{command}\"",
            WorkingDirectory = workingDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var proc = Process.Start(psi)!;
        var output = new StringBuilder();
        var error = new StringBuilder();

        proc.OutputDataReceived += (_, e) => { if (e.Data is not null) lock (output) output.AppendLine(e.Data); };
        proc.ErrorDataReceived += (_, e) => { if (e.Data is not null) lock (error) error.AppendLine(e.Data); };

        proc.BeginOutputReadLine();
        proc.BeginErrorReadLine();

        if (!proc.WaitForExit(timeoutMs))
        {
            try { proc.Kill(entireProcessTree: true); } catch { }
            return (-1, $"TIMEOUT after {timeoutMs}ms");
        }

        return (proc.ExitCode, output.ToString() + error.ToString());
    }

    // ── Test 1: Development build output matches UE ────────

    private static void TestDevelopmentBuildOutputMatchesUE()
    {
        Console.WriteLine("[Test 1] Development build output matches UE flat layout");

        var result = RunBuild(BuildConfiguration.Development);
        Assert(result.Success, $"Build should succeed: {result.Message} | {result.ErrorDetail}");
        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist");

        // EXE(s)
        var exeFiles = Directory.GetFiles(_outputDir, "*.exe");
        Assert(exeFiles.Length > 0, "Should have at least one .exe");

        // Module DLLs (Development = modular)
        var dllFiles = Directory.GetFiles(_outputDir, "*.dll");
        Assert(dllFiles.Length > 0, "Should have module DLLs in Development build");

        // PDB debug symbols
        var pdbFiles = Directory.GetFiles(_outputDir, "*.pdb");
        Assert(pdbFiles.Length > 0, "Should have PDB files");

        // Manifests
        var modulesFiles = Directory.GetFiles(_outputDir, "*.modules");
        Assert(modulesFiles.Length > 0, "Should have .modules manifest");

        var targetFiles = Directory.GetFiles(_outputDir, "*.target");
        Assert(targetFiles.Length > 0, "Should have .target manifest");

        // Flat - no subdirectories
        var subdirs = Directory.GetDirectories(_outputDir);
        Assert(subdirs.Length == 0,
            $"Binaries/Win64/ should be flat (no subdirs), found: {string.Join(", ", subdirs.Select(Path.GetFileName))}");

        Console.WriteLine($"  EXE: {exeFiles.Length}, DLL: {dllFiles.Length}, PDB: {pdbFiles.Length}");
        Console.WriteLine($"  Manifests: {modulesFiles.Length} .modules, {targetFiles.Length} .target");
        Console.WriteLine("  PASSED");
    }

    // ── Test 2: Development build runs ───────────────────────

    private static void TestDevelopmentBuildRuns()
    {
        Console.WriteLine("[Test 2] Development build runs and engine initializes");

        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist from Test 1");
        var exeFiles = Directory.GetFiles(_outputDir, "*.exe");
        Assert(exeFiles.Length > 0, "No EXE found to run");

        // Pick the project EXE (engine entry point, named after project)
        string expectedExeName = $"{_projectName}.exe";
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
                // Verify engine startup - proves EXE + all DLLs loaded correctly
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

            // Wait up to 10s for engine startup output
            var sw = Stopwatch.StartNew();
            while (sw.ElapsedMilliseconds < 10_000 && !foundStartup && !proc.HasExited)
            {
                Thread.Sleep(100);
            }

            // If process exited immediately, give async readers time to flush
            if (proc.HasExited && !foundStartup)
            {
                Thread.Sleep(500);
            }

            string captured = output.ToString();
            Assert(foundStartup,
                $"Output should contain '[GuardedMain]' (engine startup), " +
                $"exitCode={(!proc.HasExited ? "running" : proc.ExitCode.ToString())}, " +
                $"got:\n{captured[..Math.Min(500, captured.Length)]}");

            Console.WriteLine($"  Engine startup confirmed");
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

    // ── Test 3: IDE build via NMake ────────────────────────

    private static void TestIDEBuildViaNMake()
    {
        Console.WriteLine("[Test 3] IDE build via NMake command from Launch.vcxproj");

        // Generate project files to produce .vcxproj
        var genResult = RunGenerateProjectFiles();
        Assert(genResult.Success, $"Generate project files should succeed: {genResult.Message}");

        string vcxprojPath = FindLaunchVcxproj();
        Console.WriteLine($"  Found: {vcxprojPath}");

        // Extract NMakeBuildCommandLine for Development
        string nmakeCmd = ParseNMakeProperty(vcxprojPath, "NMakeBuildCommandLine");
        Assert(!string.IsNullOrWhiteSpace(nmakeCmd), "NMakeBuildCommandLine should not be empty");
        Console.WriteLine($"  NMakeCmd: {nmakeCmd[..Math.Min(120, nmakeCmd.Length)]}...");

        // Clean before NMake build to prove NMake alone produces output
        RunClean();
        Assert(!Directory.Exists(_outputDir) || Directory.GetFiles(_outputDir).Length == 0,
            "Output should be clean before NMake build");

        // Execute the NMake command via shell
        var (exitCode, output) = RunShellCommand(nmakeCmd, _projectRoot, timeoutMs: 300_000);
        Assert(exitCode == 0,
            $"NMake build should succeed (exit {exitCode}):\n{output[..Math.Min(500, output.Length)]}");

        // Verify binaries produced
        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist after NMake build");
        var exeFiles = Directory.GetFiles(_outputDir, "*.exe");
        Assert(exeFiles.Length > 0, "NMake build should produce EXE");
        var dllFiles = Directory.GetFiles(_outputDir, "*.dll");
        Assert(dllFiles.Length > 0, "NMake build should produce DLLs");

        Console.WriteLine($"  NMake produced: {exeFiles.Length} EXE, {dllFiles.Length} DLL");
        Console.WriteLine("  PASSED");
    }

    // ── Test 4: NMakeOutput path alignment ───────────────────

    private static void TestNMakeOutputPathAlignment()
    {
        Console.WriteLine("[Test 4] NMakeOutput path matches actual built EXE");

        string vcxprojPath = FindLaunchVcxproj();
        string nmakeOutput = ParseNMakeProperty(vcxprojPath, "NMakeOutput");
        Assert(!string.IsNullOrWhiteSpace(nmakeOutput), "NMakeOutput should not be empty");

        // NMakeOutput is relative from Intermediate/ProjectFiles/
        string intermediateDir = Path.Combine(_projectRoot, "Intermediate", "ProjectFiles");
        string resolvedPath = Path.GetFullPath(Path.Combine(intermediateDir, nmakeOutput));
        string expectedFileName = Path.GetFileName(resolvedPath);

        Console.WriteLine($"  NMakeOutput: {nmakeOutput}");
        Console.WriteLine($"  Resolved:    {resolvedPath}");

        // Verify the file exists in Binaries/Win64/
        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist");
        string actualPath = Path.Combine(_outputDir, expectedFileName);
        Assert(File.Exists(actualPath),
            $"Expected output at {actualPath} but file not found. " +
            $"Files in output: {string.Join(", ", Directory.GetFiles(_outputDir).Select(Path.GetFileName))}");

        Console.WriteLine($"  Verified: {expectedFileName} exists");
        Console.WriteLine("  PASSED");
    }

    // ── Test 5: Incremental build ────────────────────────────

    private static void TestIncrementalBuild()
    {
        Console.WriteLine("[Test 5] Incremental build completes under 10 seconds");

        // Ensure we have a prior build (from Test 3/4)
        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist from prior build");

        // Touch a source file to trigger rebuild
        string sourceDir = Path.Combine(_projectRoot, "Source", "EnigmaArcade", "Private");
        var cppFiles = Directory.GetFiles(sourceDir, "*.cpp", SearchOption.TopDirectoryOnly);
        Assert(cppFiles.Length > 0, "Should find .cpp files to touch");

        string touchedFile = cppFiles[0];
        File.SetLastWriteTimeUtc(touchedFile, DateTime.UtcNow);
        Console.WriteLine($"  Touched: {Path.GetFileName(touchedFile)}");

        // Rebuild and measure time
        var sw = Stopwatch.StartNew();
        var result = RunBuild(BuildConfiguration.Development);
        sw.Stop();

        Assert(result.Success, $"Incremental build should succeed: {result.Message} | {result.ErrorDetail}");
        Assert(sw.Elapsed.TotalSeconds < 10,
            $"Incremental build should complete under 10s, took {sw.Elapsed.TotalSeconds:F1}s");

        Console.WriteLine($"  Incremental build: {sw.Elapsed.TotalSeconds:F1}s");
        Console.WriteLine("  PASSED");
    }

    // ── Test 6: Clean and rebuild ────────────────────────────

    private static void TestCleanAndRebuild()
    {
        Console.WriteLine("[Test 6] Clean removes artifacts, rebuild restores them");

        // Clean
        RunClean();

        // Verify artifacts removed
        bool binariesGone = !Directory.Exists(_outputDir)
            || Directory.GetFiles(_outputDir).Length == 0;
        Assert(binariesGone, "Binaries/Win64/ should be empty or gone after clean");

        string intermediateBuildDir = Path.Combine(_projectRoot, "Intermediate", "Build");
        bool intermediateGone = !Directory.Exists(intermediateBuildDir);
        Assert(intermediateGone, "Intermediate/Build/ should be gone after clean");

        Console.WriteLine("  Clean verified: artifacts removed");

        // Rebuild
        var result = RunBuild(BuildConfiguration.Development);
        Assert(result.Success, $"Rebuild should succeed: {result.Message} | {result.ErrorDetail}");

        // Verify output restored
        Assert(Directory.Exists(_outputDir), "Binaries/Win64/ should exist after rebuild");
        var exeFiles = Directory.GetFiles(_outputDir, "*.exe");
        Assert(exeFiles.Length > 0, "Rebuild should produce EXE");
        var dllFiles = Directory.GetFiles(_outputDir, "*.dll");
        Assert(dllFiles.Length > 0, "Rebuild should produce DLLs");

        var modulesFiles = Directory.GetFiles(_outputDir, "*.modules");
        Assert(modulesFiles.Length > 0, "Rebuild should produce .modules manifest");
        var targetFiles = Directory.GetFiles(_outputDir, "*.target");
        Assert(targetFiles.Length > 0, "Rebuild should produce .target manifest");

        Console.WriteLine($"  Rebuild verified: {exeFiles.Length} EXE, {dllFiles.Length} DLL");
        Console.WriteLine("  PASSED");
    }
}
