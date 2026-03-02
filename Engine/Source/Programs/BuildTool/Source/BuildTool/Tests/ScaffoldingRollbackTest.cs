using BuildTool.Scaffolding;

namespace BuildTool.Tests;

/// <summary>
/// Unit tests for <see cref="ScaffoldingRollback"/>.
/// </summary>
public static class ScaffoldingRollbackTest
{
    public static void Run()
    {
        Console.WriteLine("=== ScaffoldingRollback Tests ===");
        Console.WriteLine();

        TestRollbackDeletesCreatedFiles();
        TestRollbackDeletesCreatedDirectories();
        TestRollbackRestoresModifiedFiles();
        TestCommitPreventsRollback();
        TestDisposeTriggersRollback();
        TestRollbackOrder();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    private static void TestRollbackDeletesCreatedFiles()
    {
        Console.WriteLine("[Test 1] Rollback deletes created files");

        var root = CreateTempDir();
        try
        {
            var file1 = Path.Combine(root, "file1.txt");
            var file2 = Path.Combine(root, "file2.txt");
            File.WriteAllText(file1, "content1");
            File.WriteAllText(file2, "content2");

            var rollback = new ScaffoldingRollback();
            rollback.TrackFile(file1);
            rollback.TrackFile(file2);

            Assert(File.Exists(file1), "file1 should exist before rollback");
            rollback.Rollback();
            Assert(!File.Exists(file1), "file1 should be deleted after rollback");
            Assert(!File.Exists(file2), "file2 should be deleted after rollback");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(root); }
    }

    private static void TestRollbackDeletesCreatedDirectories()
    {
        Console.WriteLine("[Test 2] Rollback deletes created directories");

        var root = CreateTempDir();
        try
        {
            var dir = Path.Combine(root, "subdir");
            Directory.CreateDirectory(dir);

            var rollback = new ScaffoldingRollback();
            rollback.TrackDirectory(dir);

            Assert(Directory.Exists(dir), "dir should exist before rollback");
            rollback.Rollback();
            Assert(!Directory.Exists(dir), "dir should be deleted after rollback");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(root); }
    }

    private static void TestRollbackRestoresModifiedFiles()
    {
        Console.WriteLine("[Test 3] Rollback restores modified files");

        var root = CreateTempDir();
        try
        {
            var file = Path.Combine(root, "config.json");
            File.WriteAllText(file, "original content");

            var rollback = new ScaffoldingRollback();
            rollback.TrackModifiedFile(file);

            // Modify the file after tracking
            File.WriteAllText(file, "modified content");
            Assert(File.ReadAllText(file) == "modified content", "file should be modified");

            rollback.Rollback();
            var restored = File.ReadAllText(file);
            Assert(restored == "original content",
                $"file should be restored to original, got: {restored}");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(root); }
    }

    private static void TestCommitPreventsRollback()
    {
        Console.WriteLine("[Test 4] Commit prevents rollback on Dispose");

        var root = CreateTempDir();
        try
        {
            var file = Path.Combine(root, "keep.txt");
            File.WriteAllText(file, "keep me");

            using (var rollback = new ScaffoldingRollback())
            {
                rollback.TrackFile(file);
                rollback.Commit();
            } // Dispose called here - should NOT rollback

            Assert(File.Exists(file), "file should still exist after committed Dispose");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(root); }
    }

    private static void TestDisposeTriggersRollback()
    {
        Console.WriteLine("[Test 5] Dispose without Commit triggers rollback");

        var root = CreateTempDir();
        try
        {
            var file = Path.Combine(root, "temp.txt");
            File.WriteAllText(file, "temporary");

            using (var rollback = new ScaffoldingRollback())
            {
                rollback.TrackFile(file);
                // No Commit() - Dispose should trigger Rollback
            }

            Assert(!File.Exists(file), "file should be deleted after uncommitted Dispose");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(root); }
    }

    private static void TestRollbackOrder()
    {
        Console.WriteLine("[Test 6] Rollback deletes files before directories (bottom-up)");

        var root = CreateTempDir();
        try
        {
            var dir = Path.Combine(root, "parent");
            var subDir = Path.Combine(dir, "child");
            Directory.CreateDirectory(subDir);
            var file = Path.Combine(subDir, "data.txt");
            File.WriteAllText(file, "data");

            var rollback = new ScaffoldingRollback();
            rollback.TrackFile(file);
            rollback.TrackDirectory(subDir);
            rollback.TrackDirectory(dir);

            rollback.Rollback();

            Assert(!File.Exists(file), "file should be deleted");
            Assert(!Directory.Exists(subDir), "child dir should be deleted");
            Assert(!Directory.Exists(dir), "parent dir should be deleted");

            Console.WriteLine("  PASSED");
        }
        finally { Cleanup(root); }
    }

    private static string CreateTempDir()
    {
        var dir = Path.Combine(Path.GetTempPath(), "EnigmaTest_" + Guid.NewGuid().ToString("N")[..8]);
        Directory.CreateDirectory(dir);
        return dir;
    }

    private static void Cleanup(string root)
    {
        try { if (Directory.Exists(root)) Directory.Delete(root, recursive: true); }
        catch { /* Best effort */ }
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
}
