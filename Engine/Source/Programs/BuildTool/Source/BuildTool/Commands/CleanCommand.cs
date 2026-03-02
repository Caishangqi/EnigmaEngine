using BuildTool.Models;

namespace BuildTool.Commands;

/// <summary>
/// Cleans build artifacts for the specified project.
/// Deletes: Intermediate/Build/ and Binaries/ directories.
/// Does NOT delete Intermediate/ root (preserves non-build intermediates).
/// Idempotent - succeeds even if directories don't exist.
/// </summary>
public sealed class CleanCommand : ICommand
{
    public string Name => "clean";
    public string Description => "Clean build artifacts for the specified project.";

    public BuildResult Execute(BuildOptions options)
    {
        Console.WriteLine("╔══════════════════════════════════════════════╗");
        Console.WriteLine("║         EnigmaEngine Clean                  ║");
        Console.WriteLine("╚══════════════════════════════════════════════╝");
        Console.WriteLine($"  Project: {options.ProjectPath}");
        Console.WriteLine();

        string projectRoot = Path.GetFullPath(options.ProjectPath);

        // Directories to clean
        string intermediateBuildDir = Path.Combine(projectRoot, "Intermediate", "Build");
        string binariesDir = Path.Combine(projectRoot, "Binaries");

        int deletedCount = 0;

        // [1] Delete Intermediate/Build/
        deletedCount += TryDeleteDirectory(intermediateBuildDir, "Intermediate/Build");

        // [2] Delete Binaries/
        deletedCount += TryDeleteDirectory(binariesDir, "Binaries");

        // [3] Delete generated CMakeLists.txt at project root
        string cmakeListsPath = Path.Combine(projectRoot, "CMakeLists.txt");
        deletedCount += TryDeleteFile(cmakeListsPath, "CMakeLists.txt");

        Console.WriteLine();
        string summary = deletedCount > 0
            ? $"Clean complete - removed {deletedCount} item(s)."
            : "Clean complete - nothing to remove (already clean).";
        Console.WriteLine($"[Clean] {summary}");

        return BuildResult.Ok(summary);
    }

    /// <summary>
    /// Attempt to delete a directory recursively. Handles IOException for locked files.
    /// Returns 1 if deleted, 0 if not found or failed.
    /// </summary>
    private static int TryDeleteDirectory(string path, string displayName)
    {
        if (!Directory.Exists(path))
        {
            Console.WriteLine($"  [Skip] {displayName} - not found");
            return 0;
        }

        try
        {
            Directory.Delete(path, recursive: true);
            Console.WriteLine($"  [Deleted] {displayName}");
            return 1;
        }
        catch (IOException ex)
        {
            Console.Error.WriteLine($"  [Warning] Could not fully delete {displayName}: {ex.Message}");
            return 0;
        }
        catch (UnauthorizedAccessException ex)
        {
            Console.Error.WriteLine($"  [Warning] Access denied for {displayName}: {ex.Message}");
            return 0;
        }
    }

    /// <summary>
    /// Attempt to delete a single file. Handles IOException for locked files.
    /// Returns 1 if deleted, 0 if not found or failed.
    /// </summary>
    private static int TryDeleteFile(string path, string displayName)
    {
        if (!File.Exists(path))
        {
            Console.WriteLine($"  [Skip] {displayName} - not found");
            return 0;
        }

        try
        {
            File.Delete(path);
            Console.WriteLine($"  [Deleted] {displayName}");
            return 1;
        }
        catch (IOException ex)
        {
            Console.Error.WriteLine($"  [Warning] Could not delete {displayName}: {ex.Message}");
            return 0;
        }
        catch (UnauthorizedAccessException ex)
        {
            Console.Error.WriteLine($"  [Warning] Access denied for {displayName}: {ex.Message}");
            return 0;
        }
    }
}
