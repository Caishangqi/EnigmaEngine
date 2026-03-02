using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Parsers;

namespace BuildTool.Tests;

/// <summary>
/// Phase 1 end-to-end integration test.
/// Runs the full BuildTool pipeline: parse → resolve → generate → compile → execute.
/// </summary>
public static class Phase1IntegrationTest
{
    public static void Run()
    {
        Console.WriteLine("=== Phase 1 Integration Test: Hello World ===");
        Console.WriteLine();

        // Resolve the test project root (relative to build output)
        var projectRoot = ResolveProjectRoot();
        Console.WriteLine($"Project root: {projectRoot}");
        Console.WriteLine();

        // Step 1: Parse .eproject
        Console.WriteLine("[Step 1] Parsing .eproject ...");
        var eprojectPath = Path.Combine(projectRoot, "Phase1_HelloWorld.eproject");
        var project = ProjectParser.Parse(eprojectPath);
        Assert(project.Modules.Count == 1, $"Expected 1 module, got {project.Modules.Count}");
        Assert(project.Modules[0].Name == "Phase1_HelloWorld", $"Module name mismatch: {project.Modules[0].Name}");
        Console.WriteLine($"  OK: {project.Modules.Count} module(s), engine={project.EngineAssociation}");

        // Step 2: Parse .Target.cs
        Console.WriteLine("[Step 2] Parsing .Target.cs ...");
        var targetPath = Path.Combine(projectRoot, "Source", "Phase1_HelloWorld.Target.cs");
        var target = TargetParser.Parse(targetPath);
        Assert(target.Type == TargetType.Game, $"Expected Game target, got {target.Type}");
        Assert(target.ExtraModuleNames.Contains("Phase1_HelloWorld"), "ExtraModuleNames should contain Phase1_HelloWorld");
        Console.WriteLine($"  OK: type={target.Type}, extras=[{string.Join(", ", target.ExtraModuleNames)}]");

        // Step 3: Parse .Build.cs
        Console.WriteLine("[Step 3] Parsing .Build.cs ...");
        var buildCsPath = Path.Combine(projectRoot, "Source", "Phase1_HelloWorld", "Phase1_HelloWorld.Build.cs");
        var moduleRules = ModuleParser.Parse(buildCsPath);
        Assert(moduleRules.ModuleName == "Phase1_HelloWorld", $"Module name mismatch: {moduleRules.ModuleName}");
        Console.WriteLine($"  OK: module={moduleRules.ModuleName}, publicInc={moduleRules.PublicIncludePaths.Count}, privateInc={moduleRules.PrivateIncludePaths.Count}");

        // Step 4: Resolve dependencies
        Console.WriteLine("[Step 4] Resolving dependencies ...");
        var modules = new Dictionary<string, ModuleRules>
        {
            [moduleRules.ModuleName] = moduleRules,
        };
        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);
        Assert(resolveResult.Success, $"Dependency resolution failed: {resolveResult.Error}");
        Console.WriteLine($"  OK: build order = [{string.Join(", ", resolveResult.BuildOrder)}]");

        // Step 5: Generate CMakeLists.txt
        Console.WriteLine("[Step 5] Generating CMakeLists.txt ...");
        var generator = new CMakeGenerator();
        var genResult = generator.Generate("Phase1_HelloWorld", modules, resolveResult, projectRoot, target);
        Assert(genResult.Success, $"CMake generation failed: {genResult.Error}");

        // Write CMakeLists.txt to project root
        var cmakePath = Path.Combine(projectRoot, "CMakeLists.txt");
        File.WriteAllText(cmakePath, genResult.Content);
        Console.WriteLine($"  OK: wrote {genResult.Content.Length} chars to CMakeLists.txt");
        Console.WriteLine();
        Console.WriteLine("--- Generated CMakeLists.txt ---");
        Console.WriteLine(genResult.Content);
        Console.WriteLine("--- End CMakeLists.txt ---");
        Console.WriteLine();

        // Step 6: Run CMake configure + build
        Console.WriteLine("[Step 6] Running CMake configure + build ...");
        var buildDir = Path.Combine(projectRoot, "Intermediate", "Build");
        Directory.CreateDirectory(buildDir);

        var configureExitCode = RunProcess("cmake",
            $"-S \"{projectRoot}\" -B \"{buildDir}\" -G \"Visual Studio 17 2022\" -A x64",
            projectRoot);
        Assert(configureExitCode == 0, $"CMake configure failed with exit code {configureExitCode}");
        Console.WriteLine("  CMake configure: OK");

        var buildExitCode = RunProcess("cmake",
            $"--build \"{buildDir}\" --config Release",
            projectRoot);
        Assert(buildExitCode == 0, $"CMake build failed with exit code {buildExitCode}");
        Console.WriteLine("  CMake build: OK");

        // Step 7: Execute and verify output
        Console.WriteLine("[Step 7] Running executable ...");
        var exePath = FindExecutable(buildDir, "Phase1_HelloWorld");
        Assert(exePath is not null, "Cannot find Phase1_HelloWorld executable");
        Console.WriteLine($"  Executable: {exePath}");

        var (exitCode, output) = RunProcessCapture(exePath!, "", projectRoot);
        Assert(exitCode == 0, $"Executable returned exit code {exitCode}");

        var trimmedOutput = output.Trim();
        Assert(trimmedOutput == "Hello World",
            $"Expected 'Hello World', got '{trimmedOutput}'");

        Console.WriteLine($"  Output: \"{trimmedOutput}\"");
        Console.WriteLine();
        Console.WriteLine("=== Phase 1 Integration Test PASSED ===");

        // Cleanup generated files
        try { File.Delete(cmakePath); } catch { /* best effort */ }
        try { Directory.Delete(Path.Combine(projectRoot, "Intermediate"), true); } catch { /* best effort */ }
        try { Directory.Delete(Path.Combine(projectRoot, "Binaries"), true); } catch { /* best effort */ }
    }

    /// <summary>
    /// Run a process, streaming output to console. Returns exit code.
    /// </summary>
    private static int RunProcess(string fileName, string arguments, string workingDir)
    {
        var psi = new System.Diagnostics.ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            WorkingDirectory = workingDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var proc = System.Diagnostics.Process.Start(psi)!;
        // Read stdout/stderr asynchronously to avoid deadlocks
        var stdout = proc.StandardOutput.ReadToEnd();
        var stderr = proc.StandardError.ReadToEnd();
        proc.WaitForExit(120_000); // 2 minute timeout

        if (!string.IsNullOrWhiteSpace(stdout))
            Console.WriteLine(stdout);
        if (!string.IsNullOrWhiteSpace(stderr))
            Console.Error.WriteLine(stderr);

        return proc.ExitCode;
    }

    /// <summary>
    /// Run a process and capture its stdout. Returns (exitCode, stdout).
    /// </summary>
    private static (int ExitCode, string Output) RunProcessCapture(string fileName, string arguments, string workingDir)
    {
        var psi = new System.Diagnostics.ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            WorkingDirectory = workingDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var proc = System.Diagnostics.Process.Start(psi)!;
        var output = proc.StandardOutput.ReadToEnd();
        proc.WaitForExit(30_000);
        return (proc.ExitCode, output);
    }

    /// <summary>
    /// Search for the built executable in the build directory tree.
    /// </summary>
    private static string? FindExecutable(string buildDir, string name)
    {
        // Visual Studio puts binaries in config subdirectories
        var searchPatterns = new[]
        {
            Path.Combine(buildDir, "Binaries", "Release", $"{name}.exe"),
            Path.Combine(buildDir, "Binaries", $"{name}.exe"),
            Path.Combine(buildDir, "Release", $"{name}.exe"),
            Path.Combine(buildDir, $"{name}.exe"),
        };

        foreach (var path in searchPatterns)
        {
            if (File.Exists(path))
                return path;
        }

        // Fallback: recursive search
        var found = Directory.GetFiles(buildDir, $"{name}.exe", SearchOption.AllDirectories);
        return found.Length > 0 ? found[0] : null;
    }

    private static string ResolveProjectRoot()
    {
        // Navigate from build output to Tests/Phase1_HelloWorld
        var baseDir = AppContext.BaseDirectory;
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", "..", "Tests", "Phase1_HelloWorld"));

        if (Directory.Exists(candidate))
            return candidate;

        // Fallback: search upward for EnigmaEngine root
        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            var testDir = Path.Combine(dir.FullName, "Tests", "Phase1_HelloWorld");
            if (Directory.Exists(testDir))
                return testDir;
            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("Cannot find Tests/Phase1_HelloWorld project directory.");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception($"Integration test assertion failed: {message}");
        }
    }
}
