// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using BuildTool.Analysis;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Scanners;

/// <summary>
/// Scans an engine root without requiring a game project descriptor.
/// </summary>
public static class EngineScanner
{
    /// <summary>Result of an engine-only scan.</summary>
    public sealed class ScanResult
    {
        public required string EngineRoot { get; init; }
        public TargetRules? EngineTarget { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> AllModules { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> EngineModules { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> ThirdPartyModules { get; init; }
        public required PluginScanner.ScanResult PluginScanResult { get; init; }
        public required DependencyResolver.ResolveResult ResolveResult { get; init; }
    }

    /// <summary>
    /// Perform an engine-only scan from either repository root or Engine root.
    /// </summary>
    /// <param name="enginePath">Path to repository root, Engine root, or a child path inside the repository.</param>
    /// <returns>Populated engine scan result.</returns>
    /// <exception cref="DirectoryNotFoundException">If no Engine/Source/Runtime directory can be found.</exception>
    /// <exception cref="InvalidOperationException">If dependency resolution fails.</exception>
    public static ScanResult Scan(string enginePath)
    {
        string engineRoot = ResolveEngineRoot(enginePath);
        Console.WriteLine($"[EngineScanner] Engine root: {engineRoot}");

        Console.WriteLine("[EngineScanner] Parsing engine target...");
        string engineSourceDir = Path.Combine(engineRoot, "Source");
        var engineTarget = FindAndParseTarget(engineSourceDir);
        if (engineTarget is null)
        {
            Console.WriteLine("[EngineScanner] Warning: No engine Target.cs found.");
        }

        Console.WriteLine("[EngineScanner] Scanning engine modules...");
        var engineModules = ScanEngineModules(engineRoot);
        Console.WriteLine($"  Found {engineModules.Count} engine modules");

        Console.WriteLine("[EngineScanner] Scanning engine plugins...");
        string enginePluginsDir = Path.Combine(engineRoot, "Plugins");
        var pluginScanResult = Directory.Exists(enginePluginsDir)
            ? ScanAllPlugins(enginePluginsDir)
            : new PluginScanner.ScanResult();
        Console.WriteLine($"  Found {pluginScanResult.Modules.Count} plugin modules");

        Console.WriteLine("[EngineScanner] Scanning third-party modules...");
        string thirdPartyDir = Path.Combine(engineRoot, "Source", "ThirdParty");
        var thirdPartyModules = Directory.Exists(thirdPartyDir)
            ? ThirdPartyScanner.Scan(thirdPartyDir)
            : new Dictionary<string, ModuleRules>();
        Console.WriteLine($"  Found {thirdPartyModules.Count} third-party modules");

        Console.WriteLine("[EngineScanner] Resolving dependencies...");
        var allModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        foreach (var (key, value) in engineModules) allModules[key] = value;
        foreach (var (key, value) in pluginScanResult.Modules) allModules[key] = value;
        foreach (var (key, value) in thirdPartyModules) allModules[key] = value;

        var resolveResult = new DependencyResolver().Resolve(allModules);
        if (!resolveResult.Success)
        {
            throw new InvalidOperationException(
                $"Dependency resolution failed: {resolveResult.Error}");
        }
        Console.WriteLine($"  Build order: {string.Join(" -> ", resolveResult.BuildOrder)}");

        return new ScanResult
        {
            EngineRoot = engineRoot,
            EngineTarget = engineTarget,
            AllModules = allModules,
            EngineModules = engineModules,
            ThirdPartyModules = thirdPartyModules,
            PluginScanResult = pluginScanResult,
            ResolveResult = resolveResult,
        };
    }

    /// <summary>Resolve a repository or Engine path to the Engine root.</summary>
    internal static string ResolveEngineRoot(string enginePath)
    {
        string current = Path.GetFullPath(enginePath);

        if (File.Exists(current))
        {
            current = Path.GetDirectoryName(current)!;
        }

        if (IsEngineRoot(current))
        {
            return current;
        }

        string childEngine = Path.Combine(current, "Engine");
        if (IsEngineRoot(childEngine))
        {
            return childEngine;
        }

        var directory = new DirectoryInfo(current);
        while (directory is not null)
        {
            if (IsEngineRoot(directory.FullName))
            {
                return directory.FullName;
            }

            childEngine = Path.Combine(directory.FullName, "Engine");
            if (IsEngineRoot(childEngine))
            {
                return childEngine;
            }

            directory = directory.Parent;
        }

        throw new DirectoryNotFoundException(
            $"Engine root not found from '{enginePath}'. Expected Engine/Source/Runtime.");
    }

    private static bool IsEngineRoot(string path)
    {
        return Directory.Exists(Path.Combine(path, "Source", "Runtime"));
    }

    private static Dictionary<string, ModuleRules> ScanEngineModules(string engineRoot)
    {
        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);

        ScanModuleGroup(Path.Combine(engineRoot, "Source", "Runtime"), modules);
        ScanModuleGroup(Path.Combine(engineRoot, "Source", "Developer"), modules);

        return modules;
    }

    private static void ScanModuleGroup(string sourceGroupDir, Dictionary<string, ModuleRules> modules)
    {
        if (!Directory.Exists(sourceGroupDir))
        {
            return;
        }

        foreach (var moduleDir in Directory.GetDirectories(sourceGroupDir).OrderBy(Path.GetFileName))
        {
            var buildCs = Directory.GetFiles(moduleDir, "*.Build.cs", SearchOption.TopDirectoryOnly)
                .FirstOrDefault();
            if (buildCs is null)
            {
                continue;
            }

            try
            {
                var rules = ModuleParser.Parse(buildCs);
                rules.ModuleDirectory = Path.GetFullPath(moduleDir);
                modules[rules.ModuleName] = rules;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"  Warning: Failed to parse {buildCs}: {ex.Message}");
            }
        }
    }

    private static PluginScanner.ScanResult ScanAllPlugins(string pluginsRoot)
    {
        if (!Directory.Exists(pluginsRoot))
        {
            return new PluginScanner.ScanResult();
        }

        var pluginReferences = new List<PluginReference>();
        foreach (var pluginDir in Directory.GetDirectories(pluginsRoot).OrderBy(Path.GetFileName))
        {
            string pluginName = Path.GetFileName(pluginDir);
            string expectedDescriptor = Path.Combine(pluginDir, $"{pluginName}.eplugin");
            bool hasDescriptor = File.Exists(expectedDescriptor)
                || Directory.GetFiles(pluginDir, "*.eplugin", SearchOption.TopDirectoryOnly).Length > 0;
            if (!hasDescriptor)
            {
                continue;
            }

            pluginReferences.Add(new PluginReference
            {
                Name = pluginName,
                Enabled = true,
            });
        }

        return PluginScanner.Scan(pluginsRoot, pluginReferences);
    }

    private static TargetRules? FindAndParseTarget(string sourceDir)
    {
        if (!Directory.Exists(sourceDir))
        {
            return null;
        }

        foreach (var targetFile in Directory.GetFiles(sourceDir, "*.Target.cs", SearchOption.TopDirectoryOnly))
        {
            try
            {
                return TargetParser.Parse(targetFile);
            }
            catch
            {
                // Skip unparseable targets; scanner can still discover modules and tests.
            }
        }

        return null;
    }
}
