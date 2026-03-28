// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using BuildTool.Analysis;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Scanners;

/// <summary>
/// Scans a game project and its engine to discover all modules, targets, and dependencies.
/// Extracted from GenerateProjectFilesCommand to be reusable by BuildCommand and other consumers.
/// </summary>
public static class ProjectScanner
{
    /// <summary>Result of a full project scan.</summary>
    public sealed class ScanResult
    {
        public required ProjectDescriptor ProjectDescriptor { get; init; }
        public required string ProjectName { get; init; }
        public required string EprojectPath { get; init; }
        public required TargetRules GameTarget { get; init; }
        public TargetRules? EngineTarget { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> AllModules { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> EngineModules { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> GameModules { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> ThirdPartyModules { get; init; }
        public required PluginScanner.ScanResult PluginScanResult { get; init; }
        public required DependencyResolver.ResolveResult ResolveResult { get; init; }
        public required string ProjectRoot { get; init; }
        public required string EngineRoot { get; init; }
    }

    /// <summary>
    /// Perform a full project scan: resolve paths, parse descriptors and targets,
    /// scan all module categories, merge, and resolve dependencies.
    /// </summary>
    /// <param name="projectPath">Path to .eproject file or directory containing one.</param>
    /// <returns>Populated ScanResult.</returns>
    /// <exception cref="FileNotFoundException">If .eproject file cannot be found.</exception>
    /// <exception cref="DirectoryNotFoundException">If engine root cannot be found.</exception>
    /// <exception cref="InvalidOperationException">If game target or dependency resolution fails.</exception>
    public static ScanResult Scan(string projectPath)
    {
        // 1. Locate .eproject file
        string eprojectPath = ResolveProjectFile(projectPath);
        Console.WriteLine($"[ProjectScanner] Project: {eprojectPath}");

        string projectRoot = Path.GetDirectoryName(Path.GetFullPath(eprojectPath))!;
        string engineRoot = FindEngineRoot(projectRoot);
        Console.WriteLine($"[ProjectScanner] Engine root: {engineRoot}");

        // 2. Parse project descriptor
        Console.WriteLine("[ProjectScanner] Parsing project...");
        var projectDescriptor = ProjectParser.Parse(eprojectPath);
        string projectName = Path.GetFileNameWithoutExtension(eprojectPath);

        // 3. Parse target rules
        Console.WriteLine("[ProjectScanner] Parsing targets...");
        string gameSourceDir = Path.Combine(projectRoot, "Source");
        var gameTarget = FindAndParseTarget(gameSourceDir, projectName);
        if (gameTarget is null)
            throw new InvalidOperationException(
                $"Target file not found. Expected {projectName}.Target.cs in {gameSourceDir}");

        // Try to find engine target (non-fatal if missing)
        TargetRules? engineTarget = null;
        string engineSourceDir = Path.Combine(engineRoot, "Source");
        if (Directory.Exists(engineSourceDir))
        {
            engineTarget = FindAndParseTarget(engineSourceDir, null);
            if (engineTarget is null)
                Console.WriteLine("[ProjectScanner] Warning: No engine Target.cs found. " +
                    "Consider creating Engine/Source/EnigmaGame.Target.cs");
        }

        // 4. Scan engine modules
        Console.WriteLine("[ProjectScanner] Scanning engine modules...");
        var engineModules = ScanEngineModules(engineRoot);
        Console.WriteLine($"  Found {engineModules.Count} engine modules");

        // 5. Scan game modules
        Console.WriteLine("[ProjectScanner] Scanning game modules...");
        var gameModules = ScanGameModules(gameSourceDir);
        Console.WriteLine($"  Found {gameModules.Count} game modules");

        // 6. Scan plugins (project plugins + engine plugins)
        Console.WriteLine("[ProjectScanner] Scanning plugins...");
        string pluginsDir = Path.Combine(projectRoot, "Plugins");
        var pluginScanResult = Directory.Exists(pluginsDir)
            ? PluginScanner.Scan(pluginsDir, projectDescriptor.Plugins)
            : new PluginScanner.ScanResult();

        // Also scan engine plugins (Engine/Plugins/)
        string enginePluginsDir = Path.Combine(engineRoot, "Plugins");
        if (Directory.Exists(enginePluginsDir))
        {
            var enginePluginResult = PluginScanner.Scan(enginePluginsDir, projectDescriptor.Plugins);
            foreach (var (k, v) in enginePluginResult.Modules)
                pluginScanResult.Modules[k] = v;
            foreach (var (k, v) in enginePluginResult.EnabledPlugins)
                pluginScanResult.EnabledPlugins[k] = v;
            foreach (var name in enginePluginResult.DisabledPlugins)
                pluginScanResult.DisabledPlugins.Add(name);
        }

        Console.WriteLine($"  Found {pluginScanResult.Modules.Count} plugin modules");

        // 7. Scan third-party modules
        Console.WriteLine("[ProjectScanner] Scanning third-party modules...");
        string thirdPartyDir = Path.Combine(engineRoot, "Source", "ThirdParty");
        var thirdPartyModules = Directory.Exists(thirdPartyDir)
            ? ThirdPartyScanner.Scan(thirdPartyDir)
            : new Dictionary<string, ModuleRules>();
        Console.WriteLine($"  Found {thirdPartyModules.Count} third-party modules");

        // 8. Merge all modules and resolve dependencies
        Console.WriteLine("[ProjectScanner] Resolving dependencies...");
        var allModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        foreach (var (k, v) in engineModules) allModules[k] = v;
        foreach (var (k, v) in gameModules) allModules[k] = v;
        foreach (var (k, v) in pluginScanResult.Modules) allModules[k] = v;
        foreach (var (k, v) in thirdPartyModules) allModules[k] = v;

        var resolveResult = new DependencyResolver().Resolve(allModules);
        if (!resolveResult.Success)
            throw new InvalidOperationException(
                $"Dependency resolution failed: {resolveResult.Error}");
        Console.WriteLine($"  Build order: {string.Join(" → ", resolveResult.BuildOrder)}");

        return new ScanResult
        {
            ProjectDescriptor = projectDescriptor,
            ProjectName = projectName,
            EprojectPath = eprojectPath,
            GameTarget = gameTarget,
            EngineTarget = engineTarget,
            AllModules = allModules,
            EngineModules = engineModules,
            GameModules = gameModules,
            ThirdPartyModules = thirdPartyModules,
            PluginScanResult = pluginScanResult,
            ResolveResult = resolveResult,
            ProjectRoot = projectRoot,
            EngineRoot = engineRoot,
        };
    }
// PLACEHOLDER_HELPERS

    /// <summary>Resolve .eproject path: use provided path or search upward.</summary>
    internal static string ResolveProjectFile(string projectPath)
    {
        // If path points directly to an .eproject file
        if (File.Exists(projectPath) && projectPath.EndsWith(".eproject", StringComparison.OrdinalIgnoreCase))
            return Path.GetFullPath(projectPath);

        // If path is a directory, search for .eproject in it
        string searchDir = Directory.Exists(projectPath)
            ? Path.GetFullPath(projectPath)
            : Path.GetFullPath(Directory.GetCurrentDirectory());

        var searchedPaths = new List<string>();
        for (int i = 0; i < 6; i++)
        {
            searchedPaths.Add(searchDir);
            var eprojectFiles = Directory.GetFiles(searchDir, "*.eproject");
            if (eprojectFiles.Length > 0)
                return eprojectFiles[0];

            string? parent = Path.GetDirectoryName(searchDir);
            if (parent is null || parent == searchDir) break;
            searchDir = parent;
        }

        throw new FileNotFoundException(
            $".eproject file not found. Searched:\n  {string.Join("\n  ", searchedPaths)}");
    }

    /// <summary>Find engine root by looking for Engine/Source/Runtime/ directory.</summary>
    internal static string FindEngineRoot(string projectRoot)
    {
        string? current = projectRoot;
        for (int i = 0; i < 5; i++)
        {
            string? parent = Path.GetDirectoryName(current);
            if (parent is null) break;

            string engineCandidate = Path.Combine(parent, "Engine");
            if (Directory.Exists(Path.Combine(engineCandidate, "Source", "Runtime")))
                return engineCandidate;

            current = parent;
        }

        throw new DirectoryNotFoundException(
            $"Engine root not found. Expected Engine/Source/Runtime/ relative to {projectRoot}");
    }
// PLACEHOLDER_SCAN_METHODS

    /// <summary>Scan engine modules from Engine/Source/Runtime/.</summary>
    private static Dictionary<string, ModuleRules> ScanEngineModules(string engineRoot)
    {
        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        string runtimeDir = Path.Combine(engineRoot, "Source", "Runtime");
        if (!Directory.Exists(runtimeDir)) return modules;

        foreach (var moduleDir in Directory.GetDirectories(runtimeDir))
        {
            var buildCs = Directory.GetFiles(moduleDir, "*.Build.cs").FirstOrDefault();
            if (buildCs is null) continue;

            try
            {
                var rules = ModuleParser.Parse(buildCs);
                rules.ModuleDirectory = moduleDir;
                modules[rules.ModuleName] = rules;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"  Warning: Failed to parse {buildCs}: {ex.Message}");
            }
        }

        // Also scan Engine/Source/Developer/ for developer tool modules.
        string developerDir = Path.Combine(engineRoot, "Source", "Developer");
        if (Directory.Exists(developerDir))
        {
            foreach (var moduleDir in Directory.GetDirectories(developerDir))
            {
                var buildCs = Directory.GetFiles(moduleDir, "*.Build.cs").FirstOrDefault();
                if (buildCs is null) continue;

                try
                {
                    var rules = ModuleParser.Parse(buildCs);
                    rules.ModuleDirectory = moduleDir;
                    modules[rules.ModuleName] = rules;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"  Warning: Failed to parse {buildCs}: {ex.Message}");
                }
            }
        }

        return modules;
    }

    /// <summary>Scan game modules from Games/{Project}/Source/.</summary>
    private static Dictionary<string, ModuleRules> ScanGameModules(string gameSourceDir)
    {
        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        if (!Directory.Exists(gameSourceDir)) return modules;

        foreach (var moduleDir in Directory.GetDirectories(gameSourceDir))
        {
            var buildCs = Directory.GetFiles(moduleDir, "*.Build.cs").FirstOrDefault();
            if (buildCs is null) continue;

            try
            {
                var rules = ModuleParser.Parse(buildCs);
                rules.ModuleDirectory = moduleDir;
                modules[rules.ModuleName] = rules;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"  Warning: Failed to parse {buildCs}: {ex.Message}");
            }
        }

        return modules;
    }

    /// <summary>Find and parse a .Target.cs file in the given directory.</summary>
    private static TargetRules? FindAndParseTarget(string sourceDir, string? expectedName)
    {
        if (!Directory.Exists(sourceDir)) return null;

        var targetFiles = Directory.GetFiles(sourceDir, "*.Target.cs", SearchOption.TopDirectoryOnly);
        foreach (var targetFile in targetFiles)
        {
            try
            {
                var target = TargetParser.Parse(targetFile);
                if (expectedName is null || target.TargetName.Contains(expectedName, StringComparison.OrdinalIgnoreCase))
                    return target;
            }
            catch { /* Skip unparseable targets */ }
        }

        return null;
    }
}
