// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Build;

namespace BuildTool.Tests;

/// <summary>
/// Tests for CMakeInvoker.
/// Requires CMake installed on the system.
/// Uses temporary directories for configure/build, cleaned up after each test.
/// </summary>
public static class CMakeInvokerTest
{
    public static void Run()
    {
        Console.WriteLine("=== CMakeInvoker Tests ===");
        Console.WriteLine();

        TestResolveCMakePath();
        TestConfigureArguments();
        TestBuildArguments();
        TestForwardsOutput();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestResolveCMakePath()
    {
        Console.WriteLine("[Test 1] Resolve CMake path: constructor succeeds");
        var invoker = new CMakeInvoker();

        Assert(!string.IsNullOrWhiteSpace(invoker.CmakePath),
            "CmakePath should not be empty");
        Console.WriteLine($"  PASSED (cmake: {invoker.CmakePath})");
    }

    private static void TestConfigureArguments()
    {
        Console.WriteLine("[Test 2] Configure: minimal CMake project succeeds");
        string tempDir = CreateTempCMakeProject();
        try
        {
            var invoker = new CMakeInvoker();
            string buildDir = Path.Combine(tempDir, "build");
            string generator = DetectGenerator();

            var result = invoker.Configure(tempDir, buildDir, generator);

            Assert(result.Success,
                $"Configure failed (exit={result.ExitCode}, generator={generator}): {result.Output}{result.Error}");
            Assert(Directory.Exists(buildDir), "Build directory should be created");

            Console.WriteLine($"  PASSED (generator: {generator})");
        }
        finally
        {
            CleanupTempDir(tempDir);
        }
    }

    private static void TestBuildArguments()
    {
        Console.WriteLine("[Test 3] Build: after configure, build succeeds");
        string tempDir = CreateTempCMakeProject();
        try
        {
            var invoker = new CMakeInvoker();
            string buildDir = Path.Combine(tempDir, "build");
            string generator = DetectGenerator();

            var configResult = invoker.Configure(tempDir, buildDir, generator);
            Assert(configResult.Success,
                $"Configure failed (exit={configResult.ExitCode}): {configResult.Output}");

            var buildResult = invoker.Build(buildDir, "Release");
            Assert(buildResult.Success,
                $"Build failed (exit={buildResult.ExitCode}): {buildResult.Output}");

            Console.WriteLine("  PASSED");
        }
        finally
        {
            CleanupTempDir(tempDir);
        }
    }

    private static void TestForwardsOutput()
    {
        Console.WriteLine("[Test 4] Output forwarding: stdout is non-empty");
        string tempDir = CreateTempCMakeProject();
        try
        {
            var invoker = new CMakeInvoker();
            string buildDir = Path.Combine(tempDir, "build");
            string generator = DetectGenerator();

            var result = invoker.Configure(tempDir, buildDir, generator);
            Assert(result.Success,
                $"Configure failed (exit={result.ExitCode}): {result.Output}");
            Assert(!string.IsNullOrWhiteSpace(result.Output),
                "Output should be non-empty after configure");

            Console.WriteLine($"  PASSED (output length: {result.Output.Length} chars)");
        }
        finally
        {
            CleanupTempDir(tempDir);
        }
    }

    /// <summary>
    /// Detect a usable CMake generator on this system.
    /// Prefers Ninja, then NMake Makefiles (if cl.exe available), falls back to Visual Studio.
    /// </summary>
    private static string DetectGenerator()
    {
        // Try Ninja first (fast, single-config)
        if (TryRunCommand("ninja", "--version"))
            return "Ninja";

        // Try NMake (available inside VS Developer Command Prompt)
        if (TryRunCommand("nmake", "/?"))
            return "NMake Makefiles";

        // Fall back to Visual Studio generator (multi-config, always works with VS installed)
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
    /// Create a minimal CMake project in a temp directory:
    /// CMakeLists.txt + main.c that compiles to an executable.
    /// </summary>
    private static string CreateTempCMakeProject()
    {
        string tempDir = Path.Combine(Path.GetTempPath(), $"CMakeInvokerTest_{Guid.NewGuid():N}");
        Directory.CreateDirectory(tempDir);

        File.WriteAllText(Path.Combine(tempDir, "CMakeLists.txt"),
            """
            cmake_minimum_required(VERSION 3.20)
            project(TestProject C)
            add_executable(test_app main.c)
            """);

        File.WriteAllText(Path.Combine(tempDir, "main.c"),
            """
            #include <stdio.h>
            int main(void) {
                printf("Hello from CMakeInvokerTest\n");
                return 0;
            }
            """);

        return tempDir;
    }

    private static void CleanupTempDir(string path)
    {
        try { Directory.Delete(path, recursive: true); }
        catch { /* Best-effort cleanup */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
