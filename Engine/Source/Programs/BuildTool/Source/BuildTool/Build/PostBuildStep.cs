// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Scanners;
using static BuildTool.Models.BinaryNaming;

/// <summary>
/// Context for post-build binary placement and manifest generation.
/// </summary>
public sealed class PostBuildContext
{
    /// <summary>CMake build output directory (contains compiled binaries).</summary>
    public required string CmakeBuildDir { get; init; }

    /// <summary>Project name used in binary naming.</summary>
    public required string ProjectName { get; init; }

    /// <summary>Full project scan result.</summary>
    public required ProjectScanner.ScanResult ScanResult { get; init; }

    /// <summary>Build options (configuration, platform).</summary>
    public required BuildOptions BuildOptions { get; init; }
}

/// <summary>
/// Post-build step: copies compiled binaries to categorized output directories
/// and generates manifest files.
///
/// Modular (Development/DebugGame/Debug/Test):
///   - Engine module DLLs → {EngineRoot}/Binaries/{Platform}/
///   - Game module DLLs + EXE → {ProjectRoot}/Binaries/{Platform}/
///   - Plugin module DLLs → {ProjectRoot}/Plugins/{PluginName}/Binaries/{Platform}/
///   - Generates per-category .modules and .target manifests
///
/// Monolithic (Shipping):
///   - Copies only the monolithic .exe and .pdb to {ProjectRoot}/Binaries/{Platform}/
///   - Generates .target with LinkType="Monolithic"
///   - No .modules file (not needed for monolithic)
/// </summary>
public static class PostBuildStep
{
    /// <summary>
    /// Execute the post-build step: copy binaries and generate manifests.
    /// </summary>
    public static BuildResult Execute(PostBuildContext context)
    {
        var config = context.BuildOptions.Configuration;
        var platform = context.BuildOptions.Platform;
        bool isMonolithic = config == BuildConfiguration.Shipping;
        string linkType = isMonolithic ? "Monolithic" : "Modular";

        string engineOutputDir = Path.Combine(context.ScanResult.EngineRoot, "Binaries", platform);
        string gameOutputDir = Path.Combine(context.ScanResult.ProjectRoot, "Binaries", platform);

        Console.WriteLine($"[PostBuild] Mode: {linkType} ({config})");
        Console.WriteLine($"[PostBuild] Source: {context.CmakeBuildDir}");
        Console.WriteLine($"[PostBuild] Engine output: {engineOutputDir}");
        Console.WriteLine($"[PostBuild] Game output:   {gameOutputDir}");

        try
        {
            // 1. Copy binaries from CMake build dir to categorized output dirs
            int copied;
            if (isMonolithic)
            {
                Directory.CreateDirectory(gameOutputDir);

                // Clean stale modular artifacts (DLLs, .modules) from previous builds.
                // Shipping produces a single monolithic EXE — any DLLs in the output
                // directory are leftovers from earlier modular (DebugGame/Development) builds.
                CleanStaleModularArtifacts(gameOutputDir);

                copied = CopyMonolithicBinaries(context.CmakeBuildDir, gameOutputDir);
            }
            else
            {
                copied = CopyModularBinaries(context);
            }

            Console.WriteLine($"[PostBuild] Copied {copied} file(s)");

            // 2. Generate manifests via ManifestGenerator
            var manifestResult = GenerateManifests(context, linkType);
            if (!manifestResult.Success)
                return BuildResult.Fail("Manifest generation failed", manifestResult.Error);

            // 3. Write project-relative manifest files to disk
            foreach (var (relativePath, content) in manifestResult.Files)
            {
                string fullPath = Path.Combine(context.ScanResult.ProjectRoot, relativePath);
                string? dir = Path.GetDirectoryName(fullPath);
                if (dir is not null)
                    Directory.CreateDirectory(dir);

                File.WriteAllText(fullPath, content);
                Console.WriteLine($"[PostBuild] Manifest: {relativePath}");
            }

            // 4. Write engine-relative manifest files to disk
            foreach (var (relativePath, content) in manifestResult.EngineFiles)
            {
                string fullPath = Path.Combine(context.ScanResult.EngineRoot, relativePath);
                string? dir = Path.GetDirectoryName(fullPath);
                if (dir is not null)
                    Directory.CreateDirectory(dir);

                File.WriteAllText(fullPath, content);
                Console.WriteLine($"[PostBuild] Engine manifest: {relativePath}");
            }

            int totalManifests = manifestResult.Files.Count + manifestResult.EngineFiles.Count;
            return BuildResult.Ok(
                $"Post-build completed: {copied} binaries, {totalManifests} manifests ({linkType})");
        }
        catch (IOException ex)
        {
            return BuildResult.Fail(
                "Post-build failed: file may be locked by another process",
                ex.Message);
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Post-build failed", ex.Message);
        }
    }

    /// <summary>
    /// Execute hot-reload post-build: copy game/plugin DLLs with versioned names.
    /// Does NOT overwrite original DLLs (they are locked by the running engine).
    /// Does NOT regenerate manifests (engine does not re-read them during hot reload).
    /// </summary>
    public static BuildResult ExecuteHotReload(PostBuildContext context)
    {
        var config = context.BuildOptions.Configuration;
        var platform = context.BuildOptions.Platform;
        string projectName = context.ProjectName;
        var scan = context.ScanResult;

        Console.WriteLine("[PostBuild] Mode: HotReload (versioned DLL output)");

        try
        {
            // Load or create hot-reload state.
            string statePath = HotReloadState.GetStateFilePath(scan.ProjectRoot);
            var state = HotReloadState.Load(statePath);
            int suffix = state.NextSuffix;

            Console.WriteLine($"[PostBuild] Hot-reload suffix: {suffix:D4}");

            // Build the output directory map (reuse existing logic).
            var fileOutputMap = BuildOutputDirectoryMap(scan, projectName, config, platform);

            // Collect hot-reloadable module DLL names (game + project plugins only).
            var hotReloadDlls = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            foreach (var (name, _) in scan.GameModules)
            {
                var dllName = ManifestGenerator.GetDllFileName(projectName, name, config, platform);
                if (fileOutputMap.TryGetValue(dllName, out var outputDir))
                    hotReloadDlls[dllName] = outputDir;
            }

            foreach (var (pluginName, descriptor) in scan.PluginScanResult.EnabledPlugins)
            {
                // Only project plugins (not engine plugins).
                string pluginDir = Path.GetDirectoryName(descriptor.SourceFilePath)!;
                string normalizedPluginDir = pluginDir.Replace('\\', '/');
                string normalizedEngineRoot = scan.EngineRoot.Replace('\\', '/');
                if (normalizedPluginDir.StartsWith(normalizedEngineRoot, StringComparison.OrdinalIgnoreCase))
                    continue;

                foreach (var moduleDesc in descriptor.Modules)
                {
                    var dllName = ManifestGenerator.GetDllFileName(projectName, moduleDesc.Name, config, platform);
                    if (fileOutputMap.TryGetValue(dllName, out var outputDir))
                        hotReloadDlls[dllName] = outputDir;
                }
            }

            if (hotReloadDlls.Count == 0)
            {
                return BuildResult.Ok("No hot-reloadable modules found");
            }

            // Copy versioned DLLs and PDBs.
            int copied = 0;
            foreach (var (originalDllName, outputDir) in hotReloadDlls)
            {
                Directory.CreateDirectory(outputDir);

                // Find the built DLL in CMake output.
                string? builtDll = FindFiles(context.CmakeBuildDir, originalDllName).FirstOrDefault();
                if (builtDll is null)
                {
                    Console.WriteLine($"  [Skip] {originalDllName} (not found in build output)");
                    continue;
                }

                // Compute versioned filename.
                string stem = Path.GetFileNameWithoutExtension(originalDllName);
                string versionedDllName = $"{stem}-{suffix:D4}.dll";
                string versionedDllPath = Path.Combine(outputDir, versionedDllName);

                File.Copy(builtDll, versionedDllPath, overwrite: true);
                Console.WriteLine($"  [HotReload] {versionedDllName}");
                copied++;

                // Copy PDB if exists.
                string pdbName = Path.ChangeExtension(originalDllName, ".pdb");
                string? builtPdb = FindFiles(context.CmakeBuildDir, pdbName).FirstOrDefault();
                if (builtPdb is not null)
                {
                    string versionedPdbName = $"{stem}-{suffix:D4}.pdb";
                    string versionedPdbPath = Path.Combine(outputDir, versionedPdbName);
                    File.Copy(builtPdb, versionedPdbPath, overwrite: true);
                    Console.WriteLine($"  [HotReload] {versionedPdbName}");
                    copied++;
                }

                state.OriginalToVersioned[originalDllName] = versionedDllName;
            }

            // Save updated state.
            state.NextSuffix = suffix + 1;
            HotReloadState.Save(state, statePath);
            Console.WriteLine($"[PostBuild] State saved (next suffix: {state.NextSuffix:D4})");

            return BuildResult.Ok($"Hot-reload completed: {copied} file(s) with suffix -{suffix:D4}");
        }
        catch (IOException ex)
        {
            return BuildResult.Fail("Hot-reload post-build failed", ex.Message);
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Hot-reload post-build failed", ex.Message);
        }
    }

    /// <summary>
    /// Copy modular binaries to categorized output directories based on module type.
    /// Engine modules → Engine/Binaries/, Game modules → Project/Binaries/,
    /// Plugin modules → Plugins/{Name}/Binaries/.
    /// </summary>
    private static int CopyModularBinaries(PostBuildContext context)
    {
        var scan = context.ScanResult;
        var config = context.BuildOptions.Configuration;
        var platform = context.BuildOptions.Platform;
        string projectName = context.ProjectName;

        // Build filename → output directory mapping
        var fileOutputMap = BuildOutputDirectoryMap(
            scan, projectName, config, platform);

        // Ensure all output directories exist
        foreach (var dir in fileOutputMap.Values.Distinct(StringComparer.OrdinalIgnoreCase))
            Directory.CreateDirectory(dir);

        int count = 0;
        foreach (var pattern in new[] { "*.exe", "*.dll", "*.pdb" })
        {
            foreach (var file in FindFiles(context.CmakeBuildDir, pattern))
            {
                string fileName = Path.GetFileName(file);

                // For PDB files, derive the output dir from the matching DLL/EXE
                string? outputDir = null;
                if (fileName.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase))
                {
                    string dllName = Path.ChangeExtension(fileName, ".dll");
                    string exeName = Path.ChangeExtension(fileName, ".exe");
                    if (fileOutputMap.TryGetValue(dllName, out outputDir) ||
                        fileOutputMap.TryGetValue(exeName, out outputDir))
                    {
                        // Found matching binary
                    }
                }
                else
                {
                    fileOutputMap.TryGetValue(fileName, out outputDir);
                }

                if (outputDir is not null)
                {
                    CopyFileFlat(file, outputDir);
                    count++;
                }
                else
                {
                    Console.WriteLine($"  [Skip] {fileName} (no matching module)");
                }
            }
        }
        return count;
    }

    /// <summary>
    /// Build a mapping from binary filename to target output directory.
    /// Classifies each module's DLL based on EngineModules, GameModules, and PluginScanResult.
    /// </summary>
    internal static Dictionary<string, string> BuildOutputDirectoryMap(
        ProjectScanner.ScanResult scan,
        string projectName,
        BuildConfiguration config,
        string platform)
    {
        var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        string engineOutputDir = Path.Combine(scan.EngineRoot, "Binaries", platform);
        string gameOutputDir = Path.Combine(scan.ProjectRoot, "Binaries", platform);

        // Engine modules (including Launch DLL) → Engine/Binaries/
        foreach (var (name, _) in scan.EngineModules)
        {
            var dllName = ManifestGenerator.GetDllFileName(projectName, name, config, platform);
            map[dllName] = engineOutputDir;
        }

        // Third-party modules → Engine/Binaries/
        foreach (var (name, rules) in scan.ThirdPartyModules)
        {
            if (rules.IsHeaderOnly) continue;
            var dllName = ManifestGenerator.GetDllFileName(projectName, name, config, platform);
            map[dllName] = engineOutputDir;
        }

        // Game modules → Project/Binaries/
        foreach (var (name, _) in scan.GameModules)
        {
            var dllName = ManifestGenerator.GetDllFileName(projectName, name, config, platform);
            map[dllName] = gameOutputDir;
        }

        // Plugin modules → {PluginDir}/Binaries/ (engine plugins stay under Engine/, game plugins under Project/)
        foreach (var (pluginName, descriptor) in scan.PluginScanResult.EnabledPlugins)
        {
            string pluginDir = Path.GetDirectoryName(descriptor.SourceFilePath)!;
            string pluginOutputDir = Path.Combine(pluginDir, "Binaries", platform);
            foreach (var moduleDesc in descriptor.Modules)
            {
                if (scan.PluginScanResult.Modules.TryGetValue(moduleDesc.Name, out var rules) && !rules.IsHeaderOnly)
                {
                    var dllName = ManifestGenerator.GetDllFileName(projectName, moduleDesc.Name, config, platform);
                    map[dllName] = pluginOutputDir;
                }
            }
        }

        // EXE: Modular builds → Engine/Binaries/ (alongside engine DLLs, like UE)
        //       Shipping → Project/Binaries/ (monolithic, no DLL dependencies)
        var exeName = $"{BinaryNaming.GetExecutableOutputName(projectName, config, platform)}.exe";
        map[exeName] = config == BuildConfiguration.Shipping ? gameOutputDir : engineOutputDir;

        return map;
    }

    /// <summary>
    /// Copy only .exe and .pdb files (no module DLLs) for Monolithic/Shipping builds.
    /// </summary>
    private static int CopyMonolithicBinaries(string buildDir, string outputDir)
    {
        int count = 0;
        foreach (var pattern in new[] { "*.exe", "*.pdb" })
        {
            foreach (var file in FindFiles(buildDir, pattern))
            {
                CopyFileFlat(file, outputDir);
                count++;
            }
        }
        return count;
    }

    /// <summary>
    /// Remove stale DLLs and .modules manifests from the output directory.
    /// Called before Shipping (monolithic) builds to ensure no leftover
    /// artifacts from previous modular builds remain alongside the EXE.
    /// </summary>
    private static void CleanStaleModularArtifacts(string outputDir)
    {
        int removed = 0;
        foreach (var pattern in new[] { "*.dll", "*.modules", "*.target" })
        {
            foreach (var file in Directory.GetFiles(outputDir, pattern))
            {
                try
                {
                    File.Delete(file);
                    Console.WriteLine($"  [Clean] {Path.GetFileName(file)}");
                    removed++;
                }
                catch (IOException ex)
                {
                    Console.Error.WriteLine(
                        $"  [Warning] Could not remove stale {Path.GetFileName(file)}: {ex.Message}");
                }
            }
        }

        if (removed > 0)
            Console.WriteLine($"[PostBuild] Cleaned {removed} stale modular artifact(s)");
    }

    /// <summary>
    /// Recursively find files matching a pattern in the build directory,
    /// excluding CMake internal artifacts (CMakeFiles/) and MSVC compiler PDBs (vc*.pdb).
    /// </summary>
    private static string[] FindFiles(string directory, string pattern)
    {
        if (!Directory.Exists(directory))
            return Array.Empty<string>();

        return Directory.GetFiles(directory, pattern, SearchOption.AllDirectories)
            .Where(f => !IsCMakeInternalFile(f))
            .ToArray();
    }

    /// <summary>
    /// Returns true for files that are CMake/MSVC internal artifacts and should
    /// NOT be copied to the final output directory.
    /// </summary>
    private static bool IsCMakeInternalFile(string filePath)
    {
        // Exclude anything under CMakeFiles/ (e.g. CompilerIdCXX.exe)
        string normalized = filePath.Replace('\\', '/');
        if (normalized.Contains("/CMakeFiles/", StringComparison.OrdinalIgnoreCase))
            return true;

        // Exclude MSVC compiler PDBs (vc140.pdb, vc143.pdb, etc.)
        string fileName = Path.GetFileName(filePath);
        if (fileName.StartsWith("vc", StringComparison.OrdinalIgnoreCase)
            && fileName.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase)
            && fileName.Length <= 10)  // vc###.pdb pattern
            return true;

        return false;
    }

    /// <summary>
    /// Copy a single file flat (no subdirectories) to the output directory.
    /// Uses overwrite=true to handle incremental builds.
    /// </summary>
    private static void CopyFileFlat(string sourceFile, string outputDir)
    {
        string fileName = Path.GetFileName(sourceFile);
        string destPath = Path.Combine(outputDir, fileName);

        try
        {
            File.Copy(sourceFile, destPath, overwrite: true);
            Console.WriteLine($"  [Copy] {fileName}");
        }
        catch (IOException ex)
        {
            Console.WriteLine($"  [Copy] FAILED: {fileName} - {ex.Message}");
            throw;
        }
    }

    /// <summary>
    /// Generate manifest files (.modules, .target) via ManifestGenerator.
    /// Passes engine module names for engine/game .modules separation.
    /// </summary>
    private static ManifestGenerator.GenerateResult GenerateManifests(
        PostBuildContext context, string linkType)
    {
        var scan = context.ScanResult;

        // Build engine module name set (engine + third-party non-header-only)
        var engineModuleNames = new HashSet<string>(StringComparer.Ordinal);
        foreach (var name in scan.EngineModules.Keys)
            engineModuleNames.Add(name);
        foreach (var (name, rules) in scan.ThirdPartyModules)
        {
            if (!rules.IsHeaderOnly)
                engineModuleNames.Add(name);
        }

        // Build engine plugin name set (plugins whose .eplugin lives under EngineRoot)
        var enginePluginNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var engineRootNorm = Path.GetFullPath(scan.EngineRoot) + Path.DirectorySeparatorChar;
        foreach (var (pluginName, descriptor) in scan.PluginScanResult.EnabledPlugins)
        {
            var pluginPath = Path.GetFullPath(descriptor.SourceFilePath);
            if (pluginPath.StartsWith(engineRootNorm, StringComparison.OrdinalIgnoreCase))
                enginePluginNames.Add(pluginName);
        }

        var generator = new ManifestGenerator();
        return generator.Generate(
            context.ProjectName,
            scan.AllModules,
            context.BuildOptions.Configuration,
            context.BuildOptions.Platform,
            scan.GameTarget,
            scan.PluginScanResult,
            linkType: linkType,
            engineModuleNames: engineModuleNames,
            enginePluginNames: enginePluginNames);
    }
}
