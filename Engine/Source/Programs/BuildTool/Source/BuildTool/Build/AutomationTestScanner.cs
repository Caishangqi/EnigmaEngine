// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using BuildTool.Models;

/// <summary>
/// Discovers module-local automation test source files without adding them to production module sources.
/// </summary>
public static class AutomationTestScanner
{
    /// <summary>Result of automation test source discovery.</summary>
    public sealed class ScanResult
    {
        public required bool EngineMode { get; init; }
        public required string RootPath { get; init; }
        public required string EngineRoot { get; init; }
        public string? ProjectRoot { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> AllModules { get; init; }
        public required IReadOnlyList<AutomationTestSource> Sources { get; init; }
    }

    /// <summary>Scan a game project and discover module-local automation test source files.</summary>
    public static ScanResult ScanProject(string projectPath)
    {
        return Scan(ProjectScanner.Scan(projectPath));
    }

    /// <summary>Scan an engine root and discover module-local automation test source files.</summary>
    public static ScanResult ScanEngine(string enginePath)
    {
        return Scan(EngineScanner.Scan(enginePath));
    }

    /// <summary>Discover automation test source files from an existing project scan.</summary>
    public static ScanResult Scan(ProjectScanner.ScanResult projectScan)
    {
        var sources = new List<AutomationTestSource>();
        AddSources(sources, projectScan.EngineModules, AutomationTestSourceOwner.Engine);
        AddSources(sources, projectScan.GameModules, AutomationTestSourceOwner.Game);
        AddSources(sources, projectScan.PluginScanResult.Modules, AutomationTestSourceOwner.Plugin);

        return new ScanResult
        {
            EngineMode = false,
            RootPath = projectScan.ProjectRoot,
            ProjectRoot = projectScan.ProjectRoot,
            EngineRoot = projectScan.EngineRoot,
            AllModules = projectScan.AllModules,
            Sources = sources.OrderBy(source => source.ModuleName, StringComparer.Ordinal)
                .ThenBy(source => source.RelativeSourcePath, StringComparer.Ordinal)
                .ToList(),
        };
    }

    /// <summary>Discover automation test source files from an existing engine-only scan.</summary>
    public static ScanResult Scan(EngineScanner.ScanResult engineScan)
    {
        var sources = new List<AutomationTestSource>();
        AddSources(sources, engineScan.EngineModules, AutomationTestSourceOwner.Engine);
        AddSources(sources, engineScan.PluginScanResult.Modules, AutomationTestSourceOwner.Plugin);

        return new ScanResult
        {
            EngineMode = true,
            RootPath = engineScan.EngineRoot,
            EngineRoot = engineScan.EngineRoot,
            AllModules = engineScan.AllModules,
            Sources = sources.OrderBy(source => source.ModuleName, StringComparer.Ordinal)
                .ThenBy(source => source.RelativeSourcePath, StringComparer.Ordinal)
                .ToList(),
        };
    }

    private static void AddSources(
        List<AutomationTestSource> sources,
        IReadOnlyDictionary<string, ModuleRules> modules,
        AutomationTestSourceOwner owner)
    {
        foreach (var rules in modules.Values.OrderBy(module => module.ModuleName, StringComparer.Ordinal))
        {
            if (string.IsNullOrWhiteSpace(rules.ModuleDirectory))
            {
                continue;
            }

            string moduleDirectory = Path.GetFullPath(rules.ModuleDirectory);
            string testDirectory = Path.Combine(moduleDirectory, "Private", "Tests");
            if (!Directory.Exists(testDirectory))
            {
                continue;
            }

            foreach (var sourceFile in Directory.GetFiles(testDirectory, "*.cpp", SearchOption.AllDirectories)
                         .OrderBy(path => path, StringComparer.Ordinal))
            {
                sources.Add(new AutomationTestSource
                {
                    ModuleName = rules.ModuleName,
                    ModuleDirectory = moduleDirectory,
                    SourceFilePath = Path.GetFullPath(sourceFile),
                    RelativeSourcePath = NormalizeRelativePath(Path.GetRelativePath(moduleDirectory, sourceFile)),
                    Owner = owner,
                    PluginName = owner == AutomationTestSourceOwner.Plugin
                        ? TryGetPluginName(moduleDirectory)
                        : null,
                });
            }
        }
    }

    private static string NormalizeRelativePath(string path)
    {
        return path.Replace('\\', '/');
    }

    private static string? TryGetPluginName(string moduleDirectory)
    {
        var moduleDir = new DirectoryInfo(moduleDirectory);
        var sourceDir = moduleDir.Parent;
        var pluginDir = sourceDir?.Parent;
        var pluginsDir = pluginDir?.Parent;

        if (sourceDir is not null
            && pluginDir is not null
            && pluginsDir is not null
            && sourceDir.Name.Equals("Source", StringComparison.OrdinalIgnoreCase)
            && pluginsDir.Name.Equals("Plugins", StringComparison.OrdinalIgnoreCase))
        {
            return pluginDir.Name;
        }

        return null;
    }
}
