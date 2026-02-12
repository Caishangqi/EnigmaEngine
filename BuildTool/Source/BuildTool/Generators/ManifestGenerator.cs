using System.Text.Json;
using System.Text.Json.Serialization;
using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Generators;

/// <summary>
/// Generates .modules and .target JSON manifest files following REQ-015 format.
///
/// .modules — BuildId + module-name-to-DLL-filename mapping.
/// .target  — TargetName, Platform, Configuration, Version, BuildProducts.
///
/// Plugin modules get separate .modules files placed in each plugin's Binaries directory.
/// </summary>
public sealed class ManifestGenerator
{
    private static readonly JsonSerializerOptions s_jsonOptions = new()
    {
        WriteIndented = true,
        PropertyNamingPolicy = null, // PascalCase as-is
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
    };

    // ── Result type ──────────────────────────────────────────

    /// <summary>
    /// Result of manifest generation.
    /// </summary>
    public sealed class GenerateResult
    {
        /// <summary>Whether generation succeeded.</summary>
        public required bool Success { get; init; }

        /// <summary>Error message if generation failed.</summary>
        public string? Error { get; init; }

        /// <summary>
        /// Generated manifest files: relative path → JSON content.
        /// Paths use forward slashes and are relative to the project root.
        /// </summary>
        public Dictionary<string, string> Files { get; init; } = new(StringComparer.Ordinal);

        public static GenerateResult Ok(Dictionary<string, string> files) =>
            new() { Success = true, Files = files };

        public static GenerateResult Fail(string error) =>
            new() { Success = false, Error = error };
    }
    // ── Public API ────────────────────────────────────────────

    /// <summary>
    /// Generate all manifest files for a project build.
    /// </summary>
    /// <param name="projectName">Game/project name (used in DLL naming and file naming).</param>
    /// <param name="modules">All non-plugin modules (keyed by module name).</param>
    /// <param name="configuration">Build configuration.</param>
    /// <param name="platform">Target platform string (default "Win64").</param>
    /// <param name="targetRules">Optional target rules for .target file generation.</param>
    /// <param name="pluginScanResult">Optional plugin scan result for per-plugin .modules files.</param>
    /// <param name="buildId">Optional explicit BuildId. If null, generated from timestamp.</param>
    /// <returns>A <see cref="GenerateResult"/> with file path → content mappings.</returns>
    public GenerateResult Generate(
        string projectName,
        IReadOnlyDictionary<string, ModuleRules> modules,
        BuildConfiguration configuration = BuildConfiguration.Development,
        string platform = "Win64",
        TargetRules? targetRules = null,
        PluginScanner.ScanResult? pluginScanResult = null,
        string? buildId = null,
        string linkType = "Modular")
    {
        if (modules.Count == 0 && (pluginScanResult is null || pluginScanResult.Modules.Count == 0))
        {
            return GenerateResult.Fail("Cannot generate manifests: no modules provided.");
        }

        var id = buildId ?? GenerateBuildId();
        var files = new Dictionary<string, string>(StringComparer.Ordinal);

        // Collect non-plugin module names (exclude plugin modules)
        var pluginModuleNames = pluginScanResult is not null
            ? new HashSet<string>(pluginScanResult.Modules.Keys, StringComparer.Ordinal)
            : new HashSet<string>(StringComparer.Ordinal);

        var projectModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        foreach (var (name, rules) in modules)
        {
            if (!pluginModuleNames.Contains(name))
                projectModules[name] = rules;
        }
        var manifestName = GetManifestBaseName(projectName, configuration, platform);

        // 1. Project .modules file (skip for Monolithic — no separate DLLs)
        if (linkType != "Monolithic")
        {
            var modulesPath = $"Binaries/{platform}/{manifestName}.modules";
            var modulesJson = GenerateModulesJson(id, projectModules, projectName, configuration, platform);
            files[modulesPath] = modulesJson;
        }

        // 2. Project .target file
        if (targetRules is not null)
        {
            var targetPath = $"Binaries/{platform}/{manifestName}.target";
            var targetJson = GenerateTargetJson(
                id, projectName, projectModules, targetRules, configuration, platform, pluginScanResult, linkType);
            files[targetPath] = targetJson;
        }

        // 3. Per-plugin .modules files (skip for Monolithic)
        if (linkType != "Monolithic" && pluginScanResult is not null)
        {
            foreach (var (pluginName, descriptor) in pluginScanResult.EnabledPlugins)
            {
                var pluginModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
                foreach (var moduleDesc in descriptor.Modules)
                {
                    if (pluginScanResult.Modules.TryGetValue(moduleDesc.Name, out var rules))
                        pluginModules[moduleDesc.Name] = rules;
                }

                if (pluginModules.Count == 0) continue;

                var pluginModulesPath = $"Plugins/{pluginName}/Binaries/{platform}/{manifestName}.modules";
                var pluginJson = GenerateModulesJson(id, pluginModules, projectName, configuration, platform);
                files[pluginModulesPath] = pluginJson;
            }
        }

        return GenerateResult.Ok(files);
    }
    // ── .modules JSON generation ───────────────────────────────

    /// <summary>
    /// Generate .modules JSON content: BuildId + module-to-DLL mapping.
    /// </summary>
    private static string GenerateModulesJson(
        string buildId,
        IReadOnlyDictionary<string, ModuleRules> modules,
        string projectName,
        BuildConfiguration configuration,
        string platform)
    {
        var modulesMap = new SortedDictionary<string, string>(StringComparer.Ordinal);
        foreach (var (name, rules) in modules)
        {
            if (rules.IsHeaderOnly) continue;
            var dllName = GetDllFileName(projectName, name, configuration, platform);
            modulesMap[name] = dllName;
        }

        var manifest = new ModulesManifest
        {
            BuildId = buildId,
            Modules = modulesMap,
        };

        return JsonSerializer.Serialize(manifest, s_jsonOptions);
    }

    // ── .target JSON generation ─────────────────────────────────

    /// <summary>
    /// Generate .target JSON content: target metadata + build products list.
    /// </summary>
    private static string GenerateTargetJson(
        string buildId,
        string projectName,
        IReadOnlyDictionary<string, ModuleRules> projectModules,
        TargetRules targetRules,
        BuildConfiguration configuration,
        string platform,
        PluginScanner.ScanResult? pluginScanResult,
        string linkType = "Modular")
    {
        var buildProducts = new List<BuildProduct>();

        var launchExe = configuration == BuildConfiguration.Development
            ? $"{projectName}.exe"
            : $"{projectName}-{platform}-{configuration}.exe";

        if (linkType == "Monolithic")
        {
            // Monolithic: only the executable (all modules statically linked)
            buildProducts.Add(new BuildProduct
            {
                Path = launchExe,
                Type = "Executable",
            });
        }
        else
        {
            // Modular: all module DLLs
            foreach (var (name, rules) in projectModules)
            {
                if (rules.IsHeaderOnly) continue;
                buildProducts.Add(new BuildProduct
                {
                    Path = GetDllFileName(projectName, name, configuration, platform),
                    Type = "DynamicLibrary",
                });
            }

            // Add plugin module DLLs
            if (pluginScanResult is not null)
            {
                foreach (var (name, rules) in pluginScanResult.Modules)
                {
                    if (rules.IsHeaderOnly) continue;
                    buildProducts.Add(new BuildProduct
                    {
                        Path = GetDllFileName(projectName, name, configuration, platform),
                        Type = "DynamicLibrary",
                    });
                }
            }
        }

        // Sort for deterministic output
        buildProducts.Sort((a, b) => string.Compare(a.Path, b.Path, StringComparison.Ordinal));

        var target = new TargetManifest
        {
            TargetName = targetRules.TargetName.Length > 0 ? targetRules.TargetName : projectName,
            Platform = platform,
            Configuration = configuration.ToString(),
            TargetType = targetRules.Type.ToString(),
            LinkType = linkType,
            Project = $"../../{projectName}.eproject",
            Launch = launchExe,
            Version = new VersionInfo
            {
                MajorVersion = 1,
                MinorVersion = 0,
                PatchVersion = 0,
                BuildId = buildId,
            },
            BuildProducts = buildProducts,
        };

        return JsonSerializer.Serialize(target, s_jsonOptions);
    }

    // ── Naming helpers ──────────────────────────────────────────

    /// <summary>
    /// Get the DLL file name for a module following REQ-015 naming convention.
    ///   Development: {ProjectName}-{ModuleName}.dll
    ///   Others:      {ProjectName}-{ModuleName}-{Platform}-{Config}.dll
    /// </summary>
    internal static string GetDllFileName(
        string projectName, string moduleName, BuildConfiguration configuration, string platform)
    {
        if (configuration == BuildConfiguration.Development)
            return $"{projectName}-{moduleName}.dll";

        return $"{projectName}-{moduleName}-{platform}-{configuration}.dll";
    }

    /// <summary>
    /// Get the manifest base file name (without extension).
    ///   Development: {ProjectName}
    ///   Others:      {ProjectName}-{Platform}-{Config}
    /// </summary>
    internal static string GetManifestBaseName(
        string projectName, BuildConfiguration configuration, string platform)
    {
        if (configuration == BuildConfiguration.Development)
            return projectName;

        return $"{projectName}-{platform}-{configuration}";
    }

    /// <summary>
    /// Generate a BuildId from the current UTC timestamp.
    /// Format: yyyyMMddHHmmss (e.g. "20260130143022").
    /// </summary>
    internal static string GenerateBuildId()
    {
        return DateTime.UtcNow.ToString("yyyyMMddHHmmss");
    }

    // ── JSON model types ────────────────────────────────────────

    private sealed class ModulesManifest
    {
        public string BuildId { get; init; } = string.Empty;
        public SortedDictionary<string, string> Modules { get; init; } = new();
    }

    private sealed class TargetManifest
    {
        public string TargetName { get; init; } = string.Empty;
        public string Platform { get; init; } = string.Empty;
        public string Configuration { get; init; } = string.Empty;
        public string TargetType { get; init; } = string.Empty;
        public string LinkType { get; init; } = "Modular";
        public string Project { get; init; } = string.Empty;
        public string Launch { get; init; } = string.Empty;
        public VersionInfo Version { get; init; } = new();
        public List<BuildProduct> BuildProducts { get; init; } = [];
    }

    private sealed class VersionInfo
    {
        public int MajorVersion { get; init; }
        public int MinorVersion { get; init; }
        public int PatchVersion { get; init; }
        public string BuildId { get; init; } = string.Empty;
    }

    private sealed class BuildProduct
    {
        public string Path { get; init; } = string.Empty;
        public string Type { get; init; } = string.Empty;
    }
}
