// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Build;
using BuildTool.Commands;
using BuildTool.Generators;

namespace BuildTool.Tests;

/// <summary>
/// Tests for PackageCommand (output directory resolution) and PackageExecutor (file operations).
/// </summary>
public static class PackageCommandTest
{
    private static int _passed;
    private static int _total;

    public static void Run()
    {
        _passed = 0;
        _total = 0;

        Console.WriteLine("=== PackageCommandTest ===\n");

        // --- ResolveOutputDirectory tests ---
        TestCliOutputHighestPriority();
        TestEprojectStagingDirRelativePath();
        TestEprojectStagingDirAbsolutePath();
        TestDefaultPath();
        TestEmptyStringsEqualNull();

        // --- PackageExecutor tests ---
        TestGeneratedContentWrittenToDisk();
        TestSourceFileCopied();
        TestRelativeSourcePathResolved();
        TestNestedDirectoriesCreated();
        TestMissingSourceFileSkipped();
        TestFailedPlanReturnsError();
        TestManifestGenerated();

        Console.WriteLine($"\n=== PackageCommandTest: {_passed}/{_total} passed ===");
        if (_passed != _total)
            throw new Exception($"PackageCommandTest: {_total - _passed} test(s) failed.");
    }

    // ── ResolveOutputDirectory tests ──────────────────────────

    private static void TestCliOutputHighestPriority()
    {
        _total++;
        var result = PackageCommand.ResolveOutputDirectory(
            cliOutput: @"C:\MyOutput",
            eprojectStagingDir: @"D:\Other",
            projectRoot: @"E:\Project",
            platform: "Win64");

        var expected = Path.GetFullPath(@"C:\MyOutput\Win64");
        Assert(result == expected, $"TestCliOutputHighestPriority: expected '{expected}', got '{result}'");
        _passed++;
    }

    private static void TestEprojectStagingDirRelativePath()
    {
        _total++;
        var result = PackageCommand.ResolveOutputDirectory(
            cliOutput: null,
            eprojectStagingDir: "Builds/Staged",
            projectRoot: @"E:\Project",
            platform: "Win64");

        var expected = Path.GetFullPath(@"E:\Project\Builds\Staged\Win64");
        Assert(result == expected, $"TestEprojectStagingDirRelativePath: expected '{expected}', got '{result}'");
        _passed++;
    }

    private static void TestEprojectStagingDirAbsolutePath()
    {
        _total++;
        var result = PackageCommand.ResolveOutputDirectory(
            cliOutput: null,
            eprojectStagingDir: @"D:\Builds\Staged",
            projectRoot: @"E:\Project",
            platform: "Win64");

        var expected = @"D:\Builds\Staged\Win64";
        Assert(result == expected, $"TestEprojectStagingDirAbsolutePath: expected '{expected}', got '{result}'");
        _passed++;
    }

    private static void TestDefaultPath()
    {
        _total++;
        var result = PackageCommand.ResolveOutputDirectory(
            cliOutput: null,
            eprojectStagingDir: null,
            projectRoot: @"E:\Project",
            platform: "Win64");

        var expected = Path.Combine(@"E:\Project", "Saved", "StagedBuilds", "Win64");
        Assert(result == expected, $"TestDefaultPath: expected '{expected}', got '{result}'");
        _passed++;
    }

    private static void TestEmptyStringsEqualNull()
    {
        _total++;
        var result = PackageCommand.ResolveOutputDirectory(
            cliOutput: "",
            eprojectStagingDir: "  ",
            projectRoot: @"E:\Project",
            platform: "Win64");

        var expected = Path.Combine(@"E:\Project", "Saved", "StagedBuilds", "Win64");
        Assert(result == expected, $"TestEmptyStringsEqualNull: expected '{expected}', got '{result}'");
        _passed++;
    }

    // ── PackageExecutor tests ─────────────────────────────────

    private static void TestGeneratedContentWrittenToDisk()
    {
        _total++;
        var tempDir = CreateTempDir();
        try
        {
            var plan = ShippingPackager.PackagePlan.Ok("test", new List<ShippingPackager.PackageEntry>
            {
                new()
                {
                    DestinationPath = "Engine/Config/StagedBuild.ini",
                    GeneratedContent = "[StagedBuild]\nGameName=Test\n",
                    Category = ShippingPackager.EntryCategory.Config,
                },
            });

            var result = PackageExecutor.Execute(plan, tempDir, @"C:\Dummy");
            Assert(result.Success, "TestGeneratedContentWrittenToDisk: Execute failed");
            Assert(result.FilesGenerated == 1, $"TestGeneratedContentWrittenToDisk: expected 1 generated, got {result.FilesGenerated}");

            var content = File.ReadAllText(Path.Combine(tempDir, "Engine", "Config", "StagedBuild.ini"));
            Assert(content.Contains("GameName=Test"), "TestGeneratedContentWrittenToDisk: content mismatch");
            _passed++;
        }
        finally { CleanupTempDir(tempDir); }
    }

    private static void TestSourceFileCopied()
    {
        _total++;
        var tempDir = CreateTempDir();
        var sourceDir = CreateTempDir();
        try
        {
            // Create a source file
            var sourceFile = Path.Combine(sourceDir, "test.exe");
            File.WriteAllText(sourceFile, "FAKE_EXE_CONTENT");

            var plan = ShippingPackager.PackagePlan.Ok("test", new List<ShippingPackager.PackageEntry>
            {
                new()
                {
                    DestinationPath = "Game.exe",
                    SourcePath = sourceFile,
                    Category = ShippingPackager.EntryCategory.Launcher,
                },
            });

            var result = PackageExecutor.Execute(plan, tempDir, @"C:\Dummy");
            Assert(result.Success, "TestSourceFileCopied: Execute failed");
            Assert(result.FilesCopied == 1, $"TestSourceFileCopied: expected 1 copied, got {result.FilesCopied}");

            var destContent = File.ReadAllText(Path.Combine(tempDir, "Game.exe"));
            Assert(destContent == "FAKE_EXE_CONTENT", "TestSourceFileCopied: content mismatch");
            _passed++;
        }
        finally { CleanupTempDir(tempDir); CleanupTempDir(sourceDir); }
    }

    private static void TestRelativeSourcePathResolved()
    {
        _total++;
        var tempDir = CreateTempDir();
        var projectRoot = CreateTempDir();
        try
        {
            // Create source file relative to projectRoot
            var binDir = Path.Combine(projectRoot, "Binaries", "Win64");
            Directory.CreateDirectory(binDir);
            File.WriteAllText(Path.Combine(binDir, "Game.exe"), "RELATIVE_CONTENT");

            var plan = ShippingPackager.PackagePlan.Ok("test", new List<ShippingPackager.PackageEntry>
            {
                new()
                {
                    DestinationPath = "Game/Binaries/Win64/Game.exe",
                    SourcePath = "Binaries/Win64/Game.exe",
                    Category = ShippingPackager.EntryCategory.GameModule,
                },
            });

            var result = PackageExecutor.Execute(plan, tempDir, projectRoot);
            Assert(result.Success, "TestRelativeSourcePathResolved: Execute failed");
            Assert(result.FilesCopied == 1, "TestRelativeSourcePathResolved: expected 1 copied");

            var content = File.ReadAllText(Path.Combine(tempDir, "Game", "Binaries", "Win64", "Game.exe"));
            Assert(content == "RELATIVE_CONTENT", "TestRelativeSourcePathResolved: content mismatch");
            _passed++;
        }
        finally { CleanupTempDir(tempDir); CleanupTempDir(projectRoot); }
    }

    private static void TestNestedDirectoriesCreated()
    {
        _total++;
        var tempDir = CreateTempDir();
        try
        {
            var plan = ShippingPackager.PackagePlan.Ok("test", new List<ShippingPackager.PackageEntry>
            {
                new()
                {
                    DestinationPath = "a/b/c/d/file.txt",
                    GeneratedContent = "deep",
                    Category = ShippingPackager.EntryCategory.Config,
                },
            });

            var result = PackageExecutor.Execute(plan, tempDir, @"C:\Dummy");
            Assert(result.Success, "TestNestedDirectoriesCreated: Execute failed");
            Assert(File.Exists(Path.Combine(tempDir, "a", "b", "c", "d", "file.txt")),
                "TestNestedDirectoriesCreated: nested file not found");
            _passed++;
        }
        finally { CleanupTempDir(tempDir); }
    }

    private static void TestMissingSourceFileSkipped()
    {
        _total++;
        var tempDir = CreateTempDir();
        try
        {
            var plan = ShippingPackager.PackagePlan.Ok("test", new List<ShippingPackager.PackageEntry>
            {
                new()
                {
                    DestinationPath = "Missing.exe",
                    SourcePath = @"C:\NonExistent\Path\Missing.exe",
                    Category = ShippingPackager.EntryCategory.Launcher,
                },
            });

            var result = PackageExecutor.Execute(plan, tempDir, @"C:\Dummy");
            Assert(result.Success, "TestMissingSourceFileSkipped: should succeed with warning");
            Assert(result.FilesCopied == 0, "TestMissingSourceFileSkipped: expected 0 copied");
            _passed++;
        }
        finally { CleanupTempDir(tempDir); }
    }

    private static void TestFailedPlanReturnsError()
    {
        _total++;
        var plan = ShippingPackager.PackagePlan.Fail("Intentional failure");
        var result = PackageExecutor.Execute(plan, @"C:\Dummy", @"C:\Dummy");
        Assert(!result.Success, "TestFailedPlanReturnsError: should fail");
        Assert(result.Error!.Contains("Intentional failure"), $"TestFailedPlanReturnsError: unexpected error: {result.Error}");
        _passed++;
    }

    private static void TestManifestGenerated()
    {
        _total++;
        var tempDir = CreateTempDir();
        try
        {
            var plan = ShippingPackager.PackagePlan.Ok("test", new List<ShippingPackager.PackageEntry>
            {
                new()
                {
                    DestinationPath = "Game.exe",
                    GeneratedContent = "FAKE",
                    Category = ShippingPackager.EntryCategory.Launcher,
                },
                new()
                {
                    DestinationPath = "Engine/Config/Staged.ini",
                    GeneratedContent = "[Config]",
                    Category = ShippingPackager.EntryCategory.Config,
                },
            });

            var result = PackageExecutor.Execute(plan, tempDir, @"C:\Dummy");
            Assert(result.Success, "TestManifestGenerated: Execute failed");

            var manifestPath = Path.Combine(tempDir, "Manifest_NonUFSFiles_Win64.txt");
            Assert(File.Exists(manifestPath), "TestManifestGenerated: manifest not found");

            var lines = File.ReadAllLines(manifestPath);
            Assert(lines.Length == 2, $"TestManifestGenerated: expected 2 entries, got {lines.Length}");
            Assert(lines.Any(l => l.Contains("Game.exe")), "TestManifestGenerated: missing Game.exe entry");
            Assert(lines.Any(l => l.Contains("Engine/Config/Staged.ini")), "TestManifestGenerated: missing Config entry");
            _passed++;
        }
        finally { CleanupTempDir(tempDir); }
    }

    // ── Helpers ───────────────────────────────────────────────

    private static void Assert(bool condition, string message)
    {
        if (condition)
        {
            Console.WriteLine($"  PASS: {message.Split(':')[0]}");
        }
        else
        {
            Console.WriteLine($"  FAIL: {message}");
            throw new Exception(message);
        }
    }

    private static string CreateTempDir()
    {
        var path = Path.Combine(Path.GetTempPath(), "EnigmaTest_" + Guid.NewGuid().ToString("N")[..8]);
        Directory.CreateDirectory(path);
        return path;
    }

    private static void CleanupTempDir(string path)
    {
        try { if (Directory.Exists(path)) Directory.Delete(path, recursive: true); }
        catch { /* best effort */ }
    }
}