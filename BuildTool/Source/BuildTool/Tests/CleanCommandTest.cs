// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Commands;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// Tests for CleanCommand:
///   [1] Deletes Intermediate/Build/ and Binaries/ when they exist
///   [2] Idempotent — succeeds when no artifacts exist
///   [3] Reports deleted item count in result message
/// </summary>
public static class CleanCommandTest
{
    public static void Run()
    {
        Console.WriteLine("=== CleanCommand Tests ===");
        Console.WriteLine();

        TestDeletesIntermediateAndBinaries();
        TestIdempotent();
        TestReportsDeletedCount();

        Console.WriteLine();
        Console.WriteLine("=== All CleanCommand tests passed ===");
    }

    /// <summary>
    /// [Test 1] Create Intermediate/Build/ and Binaries/ with files, run clean, verify deleted.
    /// </summary>
    private static void TestDeletesIntermediateAndBinaries()
    {
        Console.WriteLine("[Test 1] Deletes Intermediate/Build/ and Binaries/");

        string tempDir = CreateTempProject();
        try
        {
            // Arrange: create artifact directories with dummy files
            string intermediateBuild = Path.Combine(tempDir, "Intermediate", "Build");
            string binaries = Path.Combine(tempDir, "Binaries");
            Directory.CreateDirectory(Path.Combine(intermediateBuild, "Development"));
            File.WriteAllText(Path.Combine(intermediateBuild, "Development", "CMakeCache.txt"), "dummy");
            Directory.CreateDirectory(Path.Combine(binaries, "Win64"));
            File.WriteAllText(Path.Combine(binaries, "Win64", "TestApp.exe"), "dummy");

            // Also create a CMakeLists.txt at project root
            File.WriteAllText(Path.Combine(tempDir, "CMakeLists.txt"), "cmake_minimum_required(VERSION 3.20)");

            // Verify they exist before clean
            Assert(Directory.Exists(intermediateBuild), "Pre: Intermediate/Build/ should exist");
            Assert(Directory.Exists(binaries), "Pre: Binaries/ should exist");
            Assert(File.Exists(Path.Combine(tempDir, "CMakeLists.txt")), "Pre: CMakeLists.txt should exist");

            // Act
            var cmd = new CleanCommand();
            var result = cmd.Execute(new BuildOptions { ProjectPath = tempDir });

            // Assert
            Assert(result.Success, "Clean should succeed");
            Assert(!Directory.Exists(intermediateBuild), "Intermediate/Build/ should be deleted");
            Assert(!Directory.Exists(binaries), "Binaries/ should be deleted");
            Assert(!File.Exists(Path.Combine(tempDir, "CMakeLists.txt")), "CMakeLists.txt should be deleted");

            // Intermediate/ root should still exist (only Build/ subdirectory was deleted)
            Assert(Directory.Exists(Path.Combine(tempDir, "Intermediate")),
                "Intermediate/ root should be preserved");

            Console.WriteLine("  PASSED");
        }
        finally
        {
            CleanupTemp(tempDir);
        }
    }

    /// <summary>
    /// [Test 2] Run clean on empty project — no artifacts to delete. Should succeed.
    /// </summary>
    private static void TestIdempotent()
    {
        Console.WriteLine("[Test 2] Idempotent — clean with no artifacts");

        string tempDir = CreateTempProject();
        try
        {
            var cmd = new CleanCommand();

            // First clean — nothing to delete
            var result1 = cmd.Execute(new BuildOptions { ProjectPath = tempDir });
            Assert(result1.Success, "First clean should succeed");

            // Second clean — still nothing
            var result2 = cmd.Execute(new BuildOptions { ProjectPath = tempDir });
            Assert(result2.Success, "Second clean should succeed");
            Assert(result2.Message.Contains("already clean"), "Should report already clean");

            Console.WriteLine("  PASSED");
        }
        finally
        {
            CleanupTemp(tempDir);
        }
    }

    /// <summary>
    /// [Test 3] Verify result message reports correct deleted count.
    /// </summary>
    private static void TestReportsDeletedCount()
    {
        Console.WriteLine("[Test 3] Reports deleted item count");

        string tempDir = CreateTempProject();
        try
        {
            // Arrange: create only Binaries/ (not Intermediate/Build/)
            string binaries = Path.Combine(tempDir, "Binaries");
            Directory.CreateDirectory(binaries);

            var cmd = new CleanCommand();
            var result = cmd.Execute(new BuildOptions { ProjectPath = tempDir });

            Assert(result.Success, "Clean should succeed");
            Assert(result.Message.Contains("1 item(s)"), $"Should report 1 item deleted, got: {result.Message}");

            // Now create both directories + CMakeLists.txt
            Directory.CreateDirectory(Path.Combine(tempDir, "Intermediate", "Build"));
            Directory.CreateDirectory(Path.Combine(tempDir, "Binaries"));
            File.WriteAllText(Path.Combine(tempDir, "CMakeLists.txt"), "dummy");

            var result2 = cmd.Execute(new BuildOptions { ProjectPath = tempDir });
            Assert(result2.Success, "Clean should succeed");
            Assert(result2.Message.Contains("3 item(s)"), $"Should report 3 items deleted, got: {result2.Message}");

            Console.WriteLine("  PASSED");
        }
        finally
        {
            CleanupTemp(tempDir);
        }
    }

    // ── Helpers ──────────────────────────────────────────────

    private static string CreateTempProject()
    {
        string dir = Path.Combine(Path.GetTempPath(), $"CleanCommandTest_{Guid.NewGuid():N}");
        Directory.CreateDirectory(dir);
        return dir;
    }

    private static void CleanupTemp(string path)
    {
        try { if (Directory.Exists(path)) Directory.Delete(path, true); }
        catch { /* best effort */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new InvalidOperationException($"Assertion failed: {message}");
    }
}
