using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Generators;

/// <summary>
/// Creates a distributable Shipping directory structure following REQ-015.
///
/// Output layout:
///   {GameName}_Shipping/
///   ├── {GameName}.exe                              # Root launcher
///   ├── Engine/Binaries/Win64/                      # Engine module DLLs
///   ├── Engine/Binaries/ThirdParty/                 # Third-party runtime DLLs
///   ├── {GameName}/Binaries/Win64/                  # Game module DLLs + game exe
///   └── {GameName}/Binaries/Win64/{GameName}.target # Manifests
///
/// Produces a <see cref="PackagePlan"/> - a list of file entries describing
/// what to place where. The plan is pure data; callers decide how to execute
/// (copy, hardlink, etc.).
/// </summary>
public sealed class ShippingPackager
{
    // ── Result types ─────────────────────────────────────────

    /// <summary>
    /// A single entry in the package plan.
    /// </summary>
    public sealed class PackageEntry
    {
        /// <summary>Relative destination path within the output root (forward slashes).</summary>
        public required string DestinationPath { get; init; }

        /// <summary>
        /// Source description. For real files this is the absolute source path.
        /// For generated content (manifests), this is null and <see cref="GeneratedContent"/> is set.
        /// </summary>
        public string? SourcePath { get; init; }

        /// <summary>Generated file content (for manifests, configs). Null for copied files.</summary>
        public string? GeneratedContent { get; init; }

        /// <summary>Entry category for filtering/reporting.</summary>
        public EntryCategory Category { get; init; }
    }

    public enum EntryCategory
    {
        Launcher,
        EngineModule,
        GameModule,
        PluginModule,
        ThirdPartyRuntime,
        Manifest,
        Config,
    }
    /// <summary>
    /// Result of packaging plan generation.
    /// </summary>
    public sealed class PackagePlan
    {
        /// <summary>Whether plan generation succeeded.</summary>
        public required bool Success { get; init; }

        /// <summary>Error message if generation failed.</summary>
        public string? Error { get; init; }

        /// <summary>Root output directory name (e.g. "{GameName}_Shipping").</summary>
        public string OutputDirectoryName { get; init; } = string.Empty;

        /// <summary>All file entries in the package.</summary>
        public List<PackageEntry> Entries { get; init; } = [];

        public static PackagePlan Ok(string outputDir, List<PackageEntry> entries) =>
            new() { Success = true, OutputDirectoryName = outputDir, Entries = entries };

        public static PackagePlan Fail(string error) =>
            new() { Success = false, Error = error };
    }

    // ── Public API ───────────────────────────────────────────

    /// <summary>
    /// Generate a packaging plan for a Shipping build.
    /// </summary>
    /// <param name="projectName">Game/project name.</param>
    /// <param name="modules">All modules (engine + game), keyed by name.</param>
    /// <param name="targetRules">Target rules (must be Game type).</param>
    /// <param name="platform">Target platform (default "Win64").</param>
    /// <param name="pluginScanResult">Optional plugin scan result.</param>
    /// <param name="engineModuleNames">
    /// Names of modules that belong to the engine (placed under Engine/Binaries/).
    /// Modules not in this set and not in plugins are treated as game modules.
    /// </param>
    /// <param name="buildId">Optional explicit BuildId for manifests.</param>
    /// <param name="launcherExePath">
    /// Optional path to a pre-compiled launcher stub EXE (from LauncherStubBuilder).
    /// When provided, the root launcher entry uses this path instead of copying the game EXE.
    /// </param>
    /// <returns>A <see cref="PackagePlan"/> describing the output structure.</returns>
    public PackagePlan GeneratePlan(
        string projectName,
        IReadOnlyDictionary<string, ModuleRules> modules,
        TargetRules targetRules,
        string platform = "Win64",
        PluginScanner.ScanResult? pluginScanResult = null,
        IReadOnlySet<string>? engineModuleNames = null,
        string? buildId = null,
        string? launcherExePath = null,
        string? engineRoot = null,
        string? projectRoot = null)
    {
        if (modules.Count == 0)
            return PackagePlan.Fail("Cannot package: no modules provided.");

        if (targetRules.Type != TargetType.Game)
            return PackagePlan.Fail("Shipping packaging is only supported for Game targets.");

        var config = BuildConfiguration.Shipping;
        var id = buildId ?? ManifestGenerator.GenerateBuildId();
        var outputDir = $"{projectName}_Shipping";
        var entries = new List<PackageEntry>();

        var engineSet = engineModuleNames ?? new HashSet<string>(StringComparer.Ordinal);
        var pluginModuleNames = pluginScanResult is not null
            ? new HashSet<string>(pluginScanResult.Modules.Keys, StringComparer.Ordinal)
            : new HashSet<string>(StringComparer.Ordinal);

        // Categorize modules
        var engineModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        var gameModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);

        foreach (var (name, rules) in modules)
        {
            if (rules.IsHeaderOnly) continue;
            if (pluginModuleNames.Contains(name)) continue;

            if (engineSet.Contains(name))
                engineModules[name] = rules;
            else
                gameModules[name] = rules;
        }
        // 1. Root launcher executable
        var launcherName = $"{projectName}.exe";
        var gameBinDir = $"{projectName}/Binaries/{platform}";
        var gameExeName = $"{projectName}-{platform}-{config}.exe";

        entries.Add(new PackageEntry
        {
            DestinationPath = launcherName,
            SourcePath = launcherExePath ?? $"Binaries/{platform}/{gameExeName}",
            Category = EntryCategory.Launcher,
        });

        // 2. Game executable in {GameName}/Binaries/Win64/
        entries.Add(new PackageEntry
        {
            DestinationPath = $"{gameBinDir}/{gameExeName}",
            SourcePath = $"Binaries/{platform}/{gameExeName}",
            Category = EntryCategory.GameModule,
        });

        // 3. Engine module DLLs → Engine/Binaries/Win64/
        var engineBinDir = $"Engine/Binaries/{platform}";
        foreach (var (name, _) in engineModules)
        {
            var dllName = ManifestGenerator.GetDllFileName(projectName, name, config, platform);
            entries.Add(new PackageEntry
            {
                DestinationPath = $"{engineBinDir}/{dllName}",
                SourcePath = $"Binaries/{platform}/{dllName}",
                Category = EntryCategory.EngineModule,
            });
        }

        // 4. Game module DLLs - skipped for Shipping (monolithic).
        //    All game modules are statically linked into the monolithic EXE.
        //    Only engine module DLLs are placed separately under Engine/Binaries/.

        // 5. Plugin module DLLs → Engine/Plugins/{PluginName}/Binaries/Win64/
        if (pluginScanResult is not null)
        {
            foreach (var (pluginName, descriptor) in pluginScanResult.EnabledPlugins)
            {
                foreach (var moduleDesc in descriptor.Modules)
                {
                    if (!pluginScanResult.Modules.TryGetValue(moduleDesc.Name, out var rules))
                        continue;
                    if (rules.IsHeaderOnly) continue;

                    var dllName = ManifestGenerator.GetDllFileName(
                        projectName, moduleDesc.Name, config, platform);
                    entries.Add(new PackageEntry
                    {
                        DestinationPath = $"Engine/Plugins/{pluginName}/Binaries/{platform}/{dllName}",
                        SourcePath = $"Binaries/{platform}/{dllName}",
                        Category = EntryCategory.PluginModule,
                    });
                }
            }
        }
        // 6. Generate manifests via ManifestGenerator
        var allNonPluginModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        foreach (var (n, r) in engineModules) allNonPluginModules[n] = r;
        foreach (var (n, r) in gameModules) allNonPluginModules[n] = r;

        var manifestGen = new ManifestGenerator();
        var manifestResult = manifestGen.Generate(
            projectName, modules, config, platform, targetRules, pluginScanResult, id);

        if (manifestResult.Success)
        {
            foreach (var (relPath, content) in manifestResult.Files)
            {
                // Place project manifests under {GameName}/Binaries/Win64/
                // Plugin manifests stay under their plugin path
                string destPath;
                if (relPath.StartsWith("Plugins/", StringComparison.Ordinal))
                    destPath = $"Engine/{relPath}";
                else if (relPath.StartsWith("Binaries/", StringComparison.Ordinal))
                    destPath = $"{projectName}/{relPath}";
                else
                    destPath = relPath;

                entries.Add(new PackageEntry
                {
                    DestinationPath = destPath,
                    GeneratedContent = content,
                    Category = EntryCategory.Manifest,
                });
            }
        }

        // 7. Engine Config files → Engine/Config/ (mirrors UE staging)
        if (engineRoot is not null)
        {
            var engineConfigDir = Path.Combine(engineRoot, "Config");
            if (Directory.Exists(engineConfigDir))
            {
                foreach (var iniFile in Directory.GetFiles(engineConfigDir, "*.ini"))
                {
                    var fileName = Path.GetFileName(iniFile);
                    entries.Add(new PackageEntry
                    {
                        DestinationPath = $"Engine/Config/{fileName}",
                        SourcePath = iniFile,
                        Category = EntryCategory.Config,
                    });
                }
            }
        }

        // 8. Project Config files → {ProjectName}/Config/ (mirrors UE staging)
        if (projectRoot is not null)
        {
            var projectConfigDir = Path.Combine(projectRoot, "Config");
            if (Directory.Exists(projectConfigDir))
            {
                foreach (var iniFile in Directory.GetFiles(projectConfigDir, "*.ini"))
                {
                    var fileName = Path.GetFileName(iniFile);
                    entries.Add(new PackageEntry
                    {
                        DestinationPath = $"{projectName}/Config/{fileName}",
                        SourcePath = iniFile,
                        Category = EntryCategory.Config,
                    });
                }
            }
        }

        // 9. Plugin Config files → Engine/Plugins/{PluginName}/Config/ (mirrors UE staging)
        if (pluginScanResult is not null)
        {
            foreach (var (pluginName, descriptor) in pluginScanResult.EnabledPlugins)
            {
                var pluginDir = Path.GetDirectoryName(descriptor.SourceFilePath);
                if (pluginDir is null) continue;

                var pluginConfigDir = Path.Combine(pluginDir, "Config");
                if (!Directory.Exists(pluginConfigDir)) continue;

                foreach (var iniFile in Directory.GetFiles(pluginConfigDir, "*.ini"))
                {
                    var fileName = Path.GetFileName(iniFile);
                    entries.Add(new PackageEntry
                    {
                        DestinationPath = $"Engine/Plugins/{pluginName}/Config/{fileName}",
                        SourcePath = iniFile,
                        Category = EntryCategory.Config,
                    });
                }
            }
        }

        // 10. Engine config placeholder
        entries.Add(new PackageEntry
        {
            DestinationPath = $"Engine/Config/StagedBuild_{projectName}.ini",
            GeneratedContent = $"[StagedBuild]\nGameName={projectName}\nPlatform={platform}\nConfiguration={config}\n",
            Category = EntryCategory.Config,
        });

        // Sort entries for deterministic output
        entries.Sort((a, b) => string.Compare(a.DestinationPath, b.DestinationPath, StringComparison.Ordinal));

        return PackagePlan.Ok(outputDir, entries);
    }
}
