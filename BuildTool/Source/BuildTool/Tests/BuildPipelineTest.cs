// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Analysis;
using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Tests for BuildPipeline orchestrator.
/// Requires CMake installed on the system.
///
///   [1] Full pipeline orchestration with minimal compilable project
///   [2] Skip reconfigure when CMakeCache.txt is up-to-date
///   [3] Fails on CMake configure error (invalid source directory)
///   [4] Shipping passes BUILD_SHARED_LIBS=OFF
///   [5] Development passes BUILD_SHARED_LIBS=ON
/// </summary>
public static class BuildPipelineTest
{
    public static void Run()
    {
        Console.WriteLine("=== BuildPipeline Tests ===");
        Console.WriteLine();

        TestFullPipelineOrchestration();
        TestSkipReconfigureWhenCached();
        TestFailsOnCMakeConfigureError();
        TestShippingUsesStaticLibs();
        TestDevelopmentUsesDlls();

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
    /// Detect a usable CMake generator on this system.
    /// </summary>
    private static string DetectGenerator()
    {
        if (TryRunCommand("ninja", "--version"))
            return "Ninja";
        if (TryRunCommand("nmake", "/?"))
            return "NMake Makefiles";
        return "Visual Studio 17 2022";
    }

    private static bool TryRunCommand(string fileName, string arguments)
    {
        try
        {
            var psi = new System.Diagnostics.ProcessStartInfo
            {
                FileName = fileName,
                Arguments = arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };
            using var proc = System.Diagnostics.Process.Start(psi)!;
            proc.WaitForExit(3_000);
            return proc.ExitCode == 0;
        }
        catch { return false; }
    }

    /// <summary>
    /// Create a minimal compilable C++ project in a temp directory.
    /// Structure:
    ///   {root}/Source/TestApp/Public/  (empty)
    ///   {root}/Source/TestApp/Private/main.cpp
    /// </summary>
    private static string CreateTempProject()
    {
        string root = Path.Combine(Path.GetTempPath(), $"BuildPipelineTest_{Guid.NewGuid():N}");
        string sourceDir = Path.Combine(root, "Source", "TestApp");
        Directory.CreateDirectory(Path.Combine(sourceDir, "Public"));
        Directory.CreateDirectory(Path.Combine(sourceDir, "Private"));

        File.WriteAllText(Path.Combine(sourceDir, "Private", "main.cpp"),
            """
            #include <cstdio>
            int main() {
                printf("BuildPipelineTest OK\n");
                return 0;
            }
            """);

        return root;
    }

    /// <summary>
    /// Create a ScanResult for the minimal temp project with one executable module.
    /// </summary>
    private static ProjectScanner.ScanResult MakeScanResult(string projectRoot)
    {
        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["TestApp"] = new()
            {
                ModuleName = "TestApp",
                ModuleDirectory = Path.Combine(projectRoot, "Source", "TestApp"),
            },
        };

        var gameTarget = new TargetRules
        {
            TargetName = "TestProject",
            Type = TargetType.Game,
            ExtraModuleNames = { "TestApp" },
        };

        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);

        return new ProjectScanner.ScanResult
        {
            ProjectDescriptor = new ProjectDescriptor
            {
                FileVersion = 1,
                Modules = { new ModuleDescriptor { Name = "TestApp" } },
            },
            ProjectName = "TestProject",
            EprojectPath = Path.Combine(projectRoot, "TestProject.eproject"),
            GameTarget = gameTarget,
            AllModules = modules,
            EngineModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal),
            GameModules = modules,
            ThirdPartyModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal),
            PluginScanResult = new PluginScanner.ScanResult(),
            ResolveResult = resolveResult,
            ProjectRoot = projectRoot,
            EngineRoot = Path.Combine(projectRoot, "..", "Engine"),
        };
    }

    private static void CleanupTempDir(string path)
    {
        try { if (Directory.Exists(path)) Directory.Delete(path, recursive: true); }
        catch { /* Best-effort cleanup */ }
    }

    // ── Test 1: Full pipeline orchestration ──────────────────

    private static void TestFullPipelineOrchestration()
    {
        Console.WriteLine("[Test 1] Full pipeline: generates, configures, builds, post-builds");
        string projectRoot = CreateTempProject();
        try
        {
            var scan = MakeScanResult(projectRoot);
            var options = new BuildOptions
            {
                ProjectPath = scan.EprojectPath,
                Configuration = BuildConfiguration.Development,
                Platform = "Win64",
            };

            var invoker = new CMakeInvoker();
            var pipeline = new BuildPipeline(invoker);
            var result = pipeline.Run(scan, options);

            Assert(result.Success, $"Pipeline should succeed: {result.Message} | {result.ErrorDetail}");

            // Verify artifacts exist
            string buildDir = Path.Combine(projectRoot, "Intermediate", "Build", "Development");
            Assert(Directory.Exists(buildDir), "Build directory should exist");
            Assert(File.Exists(Path.Combine(projectRoot, "CMakeLists.txt")), "CMakeLists.txt should exist");
            Assert(File.Exists(Path.Combine(buildDir, "CMakeCache.txt")), "CMakeCache.txt should exist");

            // Verify output binaries
            string outputDir = Path.Combine(projectRoot, "Binaries", "Win64");
            Assert(Directory.Exists(outputDir), "Output directory should exist");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupTempDir(projectRoot); }
    }

    // ── Test 2: Skip reconfigure when cached ─────────────────

    private static void TestSkipReconfigureWhenCached()
    {
        Console.WriteLine("[Test 2] Skip reconfigure: second run skips CMake configure");
        string projectRoot = CreateTempProject();
        try
        {
            var scan = MakeScanResult(projectRoot);
            var options = new BuildOptions
            {
                ProjectPath = scan.EprojectPath,
                Configuration = BuildConfiguration.Development,
                Platform = "Win64",
            };

            var invoker = new CMakeInvoker();
            var pipeline = new BuildPipeline(invoker);

            // First run: full configure + build
            var sw1 = System.Diagnostics.Stopwatch.StartNew();
            var result1 = pipeline.Run(scan, options);
            sw1.Stop();
            Assert(result1.Success, $"First run should succeed: {result1.Message} | {result1.ErrorDetail}");

            // Second run: CMakeLists.txt content unchanged → skip configure
            var sw2 = System.Diagnostics.Stopwatch.StartNew();
            var result2 = pipeline.Run(scan, options);
            sw2.Stop();
            Assert(result2.Success, $"Second run should succeed: {result2.Message} | {result2.ErrorDetail}");

            // Verify CMakeCache.txt still exists (configure was skipped, not re-run)
            string buildDir = Path.Combine(projectRoot, "Intermediate", "Build", "Development");
            Assert(File.Exists(Path.Combine(buildDir, "CMakeCache.txt")), "CMakeCache.txt should exist");

            // Second run should be faster (configure skipped)
            Console.WriteLine($"  First run:  {sw1.ElapsedMilliseconds}ms");
            Console.WriteLine($"  Second run: {sw2.ElapsedMilliseconds}ms");
            Assert(sw2.ElapsedMilliseconds <= sw1.ElapsedMilliseconds + 500,
                "Second run should not be significantly slower than first");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupTempDir(projectRoot); }
    }

    // ── Test 3: Fails on CMake configure error ───────────────

    private static void TestFailsOnCMakeConfigureError()
    {
        Console.WriteLine("[Test 3] Configure error: invalid source dir causes failure");
        string projectRoot = Path.Combine(Path.GetTempPath(), $"BuildPipelineTest_{Guid.NewGuid():N}");
        Directory.CreateDirectory(projectRoot);
        try
        {
            // Create ScanResult pointing to non-existent source files
            var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
            {
                ["BadModule"] = new()
                {
                    ModuleName = "BadModule",
                    ModuleDirectory = Path.Combine(projectRoot, "Source", "NonExistent"),
                },
            };

            var gameTarget = new TargetRules
            {
                TargetName = "BadProject",
                Type = TargetType.Game,
                ExtraModuleNames = { "BadModule" },
            };

            var resolver = new DependencyResolver();
            var resolveResult = resolver.Resolve(modules);

            var scan = new ProjectScanner.ScanResult
            {
                ProjectDescriptor = new ProjectDescriptor
                {
                    FileVersion = 1,
                    Modules = { new ModuleDescriptor { Name = "BadModule" } },
                },
                ProjectName = "BadProject",
                EprojectPath = Path.Combine(projectRoot, "BadProject.eproject"),
                GameTarget = gameTarget,
                AllModules = modules,
                EngineModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal),
                GameModules = modules,
                ThirdPartyModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal),
                PluginScanResult = new PluginScanner.ScanResult(),
                ResolveResult = resolveResult,
                ProjectRoot = projectRoot,
                EngineRoot = Path.Combine(projectRoot, "..", "Engine"),
            };

            var options = new BuildOptions
            {
                ProjectPath = scan.EprojectPath,
                Configuration = BuildConfiguration.Development,
                Platform = "Win64",
            };

            var invoker = new CMakeInvoker();
            var pipeline = new BuildPipeline(invoker);
            var result = pipeline.Run(scan, options);

            Assert(!result.Success, "Pipeline should fail with invalid source directory");
            Assert(result.Message.Contains("failed", StringComparison.OrdinalIgnoreCase),
                $"Error should mention failure: {result.Message}");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupTempDir(projectRoot); }
    }
    // PLACEHOLDER_TESTS_2

    // ── Test 4: Shipping uses BUILD_SHARED_LIBS=OFF ──────────

    private static void TestShippingUsesStaticLibs()
    {
        Console.WriteLine("[Test 4] Shipping: BUILD_SHARED_LIBS=OFF in CMakeCache");
        string projectRoot = CreateTempProject();
        try
        {
            var scan = MakeScanResult(projectRoot);
            var options = new BuildOptions
            {
                ProjectPath = scan.EprojectPath,
                Configuration = BuildConfiguration.Shipping,
                Platform = "Win64",
            };

            var invoker = new CMakeInvoker();
            var pipeline = new BuildPipeline(invoker);

            // Run pipeline — may fail at build step, but configure should succeed
            pipeline.Run(scan, options);

            // Check CMakeCache.txt for BUILD_SHARED_LIBS
            string buildDir = Path.Combine(projectRoot, "Intermediate", "Build", "Shipping");
            string cachePath = Path.Combine(buildDir, "CMakeCache.txt");
            Assert(File.Exists(cachePath), "CMakeCache.txt should exist after configure");

            string cacheContent = File.ReadAllText(cachePath);
            Assert(cacheContent.Contains("BUILD_SHARED_LIBS:BOOL=OFF"),
                "Shipping should set BUILD_SHARED_LIBS=OFF");
            Assert(!cacheContent.Contains("BUILD_SHARED_LIBS:BOOL=ON"),
                "Shipping should NOT have BUILD_SHARED_LIBS=ON");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupTempDir(projectRoot); }
    }

    // ── Test 5: Development uses BUILD_SHARED_LIBS=ON ────────

    private static void TestDevelopmentUsesDlls()
    {
        Console.WriteLine("[Test 5] Development: BUILD_SHARED_LIBS=ON in CMakeCache");
        string projectRoot = CreateTempProject();
        try
        {
            var scan = MakeScanResult(projectRoot);
            var options = new BuildOptions
            {
                ProjectPath = scan.EprojectPath,
                Configuration = BuildConfiguration.Development,
                Platform = "Win64",
            };

            var invoker = new CMakeInvoker();
            var pipeline = new BuildPipeline(invoker);

            // Run pipeline — should succeed fully
            var result = pipeline.Run(scan, options);
            Assert(result.Success, $"Development pipeline should succeed: {result.Message} | {result.ErrorDetail}");

            // Check CMakeCache.txt for BUILD_SHARED_LIBS
            string buildDir = Path.Combine(projectRoot, "Intermediate", "Build", "Development");
            string cachePath = Path.Combine(buildDir, "CMakeCache.txt");
            Assert(File.Exists(cachePath), "CMakeCache.txt should exist after configure");

            string cacheContent = File.ReadAllText(cachePath);
            Assert(cacheContent.Contains("BUILD_SHARED_LIBS:BOOL=ON"),
                "Development should set BUILD_SHARED_LIBS=ON");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupTempDir(projectRoot); }
    }
}
