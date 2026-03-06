using BuildTool.Build;
using BuildTool.Models;

namespace BuildTool.Commands;

/// <summary>
/// Cleans build artifacts for the specified project.
///
/// Configuration-aware clean (default):
///   Only deletes Intermediate/Build/{Config}/ for project, engine, and plugins.
///   Preserves Binaries/ (build overwrites in-place) and other configurations.
///
/// Full clean (--full flag or used internally by RebuildCommand):
///   Deletes all Intermediate/Build/, Binaries/, ProjectFiles/, and CMakeLists.txt.
///
/// Idempotent - succeeds even if directories don't exist.
/// </summary>
public sealed class CleanCommand : ICommand
{
    /// <summary>
    /// When true, performs a full clean (all configs, Binaries, ProjectFiles).
    /// Set by RebuildCommand or --full CLI flag.
    /// </summary>
    public bool FullClean { get; set; }

    public string Name => "clean";
    public string Description => "Clean build artifacts for the specified project.";

    public BuildResult Execute(BuildOptions options)
    {
        bool isFull = FullClean
            || options.ExtraArguments.ContainsKey("full");
        string configName = options.Configuration.ToString();

        Console.WriteLine("╔══════════════════════════════════════════════╗");
        Console.WriteLine("║         EnigmaEngine Clean                  ║");
        Console.WriteLine("╚══════════════════════════════════════════════╝");
        Console.WriteLine($"  Project: {options.ProjectPath}");
        Console.WriteLine($"  Mode:    {(isFull ? "Full (all configurations)" : $"Config ({configName})")}");
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

        if (isFull)
        {
            // ── Full clean: delete everything (original behavior) ──
            deletedCount += TryDeleteDirectory(
                Path.Combine(projectRoot, "Intermediate", "Build"), "Project: Intermediate/Build");
            deletedCount += TryDeleteDirectory(
                Path.Combine(projectRoot, "Binaries"), "Project: Binaries");

            if (engineRoot is not null)
            {
                deletedCount += TryDeleteDirectory(
                    Path.Combine(engineRoot, "Intermediate", "Build"), "Engine: Intermediate/Build");
                deletedCount += TryDeleteDirectory(
                    Path.Combine(engineRoot, "Binaries"), "Engine: Binaries");
                deletedCount += TryDeleteDirectory(
                    Path.Combine(engineRoot, "Intermediate", "ProjectFiles"), "Engine: Intermediate/ProjectFiles");
            }

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

            string cmakeListsPath = Path.Combine(projectRoot, "CMakeLists.txt");
            deletedCount += TryDeleteFile(cmakeListsPath, "CMakeLists.txt");
        }
        else
        {
            // ── Config-scoped clean: only Intermediate/Build/{Config}/ ──
            deletedCount += TryDeleteDirectory(
                Path.Combine(projectRoot, "Intermediate", "Build", configName),
                $"Project: Intermediate/Build/{configName}");

            if (engineRoot is not null)
            {
                deletedCount += TryDeleteDirectory(
                    Path.Combine(engineRoot, "Intermediate", "Build", configName),
                    $"Engine: Intermediate/Build/{configName}");
            }

            if (scan?.PluginScanResult is not null)
            {
                string pluginsDir = Path.Combine(projectRoot, "Plugins");
                foreach (var (pluginName, _) in scan.PluginScanResult.EnabledPlugins)
                {
                    string pluginRoot = Path.Combine(pluginsDir, pluginName);
                    deletedCount += TryDeleteDirectory(
                        Path.Combine(pluginRoot, "Intermediate", "Build", configName),
                        $"Plugin {pluginName}: Intermediate/Build/{configName}");
                }
            }
        }

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
