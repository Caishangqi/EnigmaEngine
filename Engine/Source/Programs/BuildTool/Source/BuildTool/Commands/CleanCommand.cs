using BuildTool.Build;
using BuildTool.Models;

namespace BuildTool.Commands;

/// <summary>
/// Cleans build artifacts for the specified project.
/// Deletes: Intermediate/Build/ and Binaries/ for project, engine, and plugins.
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

        // Use ProjectScanner to resolve engine root and plugin paths
        ProjectScanner.ScanResult? scan = null;
        try
        {
            scan = ProjectScanner.Scan(options.ProjectPath);
        }
        catch
        {
            // Fallback: if scan fails, clean only project root
        }

        string projectRoot = scan?.ProjectRoot ?? Path.GetFullPath(options.ProjectPath);
        string? engineRoot = scan?.EngineRoot;

        int deletedCount = 0;

        // [1] Project: Intermediate/Build/
        deletedCount += TryDeleteDirectory(
            Path.Combine(projectRoot, "Intermediate", "Build"), "Project: Intermediate/Build");

        // [2] Project: Binaries/
        deletedCount += TryDeleteDirectory(
            Path.Combine(projectRoot, "Binaries"), "Project: Binaries");

        // [3] Engine: Intermediate/Build/
        if (engineRoot is not null)
        {
            deletedCount += TryDeleteDirectory(
                Path.Combine(engineRoot, "Intermediate", "Build"), "Engine: Intermediate/Build");
        }

        // [4] Engine: Binaries/
        if (engineRoot is not null)
        {
            deletedCount += TryDeleteDirectory(
                Path.Combine(engineRoot, "Binaries"), "Engine: Binaries");
        }

        // [5] Engine: Intermediate/ProjectFiles/
        if (engineRoot is not null)
        {
            deletedCount += TryDeleteDirectory(
                Path.Combine(engineRoot, "Intermediate", "ProjectFiles"), "Engine: Intermediate/ProjectFiles");
        }

        // [6] Plugin: Binaries/ and Intermediate/Build/
        if (scan?.PluginScanResult is not null)
        {
            string pluginsDir = Path.Combine(projectRoot, "Plugins");
            foreach (var (pluginName, _) in scan.PluginScanResult.EnabledPlugins)
            {
                string pluginRoot = Path.Combine(pluginsDir, pluginName);
                deletedCount += TryDeleteDirectory(
                    Path.Combine(pluginRoot, "Binaries"), $"Plugin {pluginName}: Binaries");
                deletedCount += TryDeleteDirectory(
                    Path.Combine(pluginRoot, "Intermediate", "Build"), $"Plugin {pluginName}: Intermediate/Build");
            }
        }

        // [7] Delete generated CMakeLists.txt at project root
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
