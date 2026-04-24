// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using System.Diagnostics;
using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;

/// <summary>
/// Core build orchestrator - single entry point for BuildCommand.
///
/// Pipeline steps:
///   [1] Prepare build directory: {ProjectRoot}/Intermediate/Build/{Config}/
///   [2] CMakeGenerator.Generate() → write CMakeLists.txt
///   [3] Check CMakeCache.txt for incremental reconfigure skip
///   [4] CMakeInvoker.Configure() with BUILD_SHARED_LIBS=ON|OFF
///   [5] CMakeInvoker.Build()
///   [6] PostBuildStep.Execute() → Binaries/Win64/
///   [7] Print build summary with elapsed time
///
/// Fails fast on any step failure. Logs each step.
/// </summary>
public sealed class BuildPipeline
{
    private readonly CMakeInvoker _cmake;

    /// <summary>
    /// Creates a new BuildPipeline with the given CMake invoker.
    /// </summary>
    /// <param name="cmake">CMake invoker for configure/build steps.</param>
    public BuildPipeline(CMakeInvoker cmake)
    {
        _cmake = cmake ?? throw new ArgumentNullException(nameof(cmake));
    }

    /// <summary>
    /// Run the full build pipeline: generate → configure → build → post-build.
    /// </summary>
    /// <param name="scan">Project scan result containing modules, targets, and dependencies.</param>
    /// <param name="options">Build options (configuration, platform).</param>
    /// <returns>BuildResult indicating success or failure with details.</returns>
    public BuildResult Run(ProjectScanner.ScanResult scan, BuildOptions options)
    {
        var sw = Stopwatch.StartNew();
        var config = options.Configuration;
        var platform = options.Platform;
        bool isShipping = config == BuildConfiguration.Shipping;

        Console.WriteLine("╔══════════════════════════════════════════════╗");
        Console.WriteLine("║       EnigmaEngine Build Pipeline            ║");
        Console.WriteLine("╚══════════════════════════════════════════════╝");
        Console.WriteLine($"  Project:       {scan.ProjectName}");
        Console.WriteLine($"  Configuration: {config}");
        Console.WriteLine($"  Platform:      {platform}");
        Console.WriteLine($"  Link mode:     {(isShipping ? "Monolithic" : "Modular")}");
        Console.WriteLine($"  Modules:       {scan.AllModules.Count}");
        Console.WriteLine();

        // [1] Prepare build directory
        string buildDir = Path.Combine(scan.ProjectRoot, "Intermediate", "Build", config.ToString());
        Directory.CreateDirectory(buildDir);
        Console.WriteLine($"[Step 1/6] Build directory: {buildDir}");

        // Early hot-reload detection (before CMake generation so PDB_NAME can be set)
        bool isHotReload = options.ExtraArguments.ContainsKey("hot-reload");
        if (!isHotReload)
        {
            string engineOutputDir = Path.Combine(scan.EngineRoot, "Binaries", options.Platform);
            string exeName = $"{BinaryNaming.GetExecutableOutputName(scan.ProjectName, options.Configuration, options.Platform)}.exe";
            string exePath = Path.Combine(engineOutputDir, exeName);
            if (File.Exists(exePath) && IsFileLocked(exePath))
            {
                Console.WriteLine("[Build] Engine exe is locked (running) -- auto-switching to hot-reload mode");
                isHotReload = true;
            }
        }

        // Collect hot-reloadable module names (always, for stable CMakeLists.txt generation)
        var hotReloadModuleNames = CollectHotReloadModuleNames(scan);

        // Load hot-reload state for PDB versioning
        int hotReloadSuffix = 0;
        if (isHotReload)
        {
            string statePath = HotReloadState.GetStateFilePath(scan.ProjectRoot);
            var state = HotReloadState.Load(statePath);
            hotReloadSuffix = state.NextSuffix;

            // Skip suffixes whose PDB files are locked in the intermediate build directory
            // (e.g., debugger holding PDBs from a previous hot-reload session).
            hotReloadSuffix = FindAvailableHotReloadSuffix(buildDir, hotReloadSuffix);

            // Persist the advanced suffix so PostBuildStep uses the same value.
            if (hotReloadSuffix != state.NextSuffix)
            {
                Console.WriteLine($"[Build] Suffix {state.NextSuffix:D4} has locked PDB(s), advanced to {hotReloadSuffix:D4}");
                state.NextSuffix = hotReloadSuffix;
                HotReloadState.Save(state, statePath);
            }

            Console.WriteLine($"[Build] Hot-reload suffix: {hotReloadSuffix:D4}, modules: {string.Join(", ", hotReloadModuleNames)}");
        }

        // [2] Generate CMakeLists.txt
        Console.WriteLine("[Step 2/6] Generating CMakeLists.txt ...");
        var genResult = GenerateCMake(scan, config, platform, hotReloadModuleNames);
        if (!genResult.Success)
            return Fail("CMake generation failed", genResult.Error, sw);

        string cmakeListsPath = Path.Combine(scan.ProjectRoot, "CMakeLists.txt");
        bool cmakeChanged = WriteIfChanged(cmakeListsPath, genResult.Content);
        Console.WriteLine(cmakeChanged
            ? $"  Written: {cmakeListsPath}"
            : $"  Unchanged: {cmakeListsPath}");

        // [3] Check CMakeCache.txt - skip reconfigure if cache exists and CMakeLists.txt is older
        bool needsConfigure = NeedsConfigure(buildDir, cmakeListsPath);

        // Hot-reload always reconfigures (HOT_RELOAD_SUFFIX changes each build)
        if (isHotReload)
            needsConfigure = true;

        // [4] CMake configure (with BUILD_SHARED_LIBS and optional HOT_RELOAD_SUFFIX)
        if (needsConfigure)
        {
            Console.WriteLine("[Step 3/6] Configuring CMake ...");
            var configureResult = ConfigureCMake(scan.ProjectRoot, buildDir, isShipping,
                isHotReload ? hotReloadSuffix : null);
            if (!configureResult.Success)
                return Fail("CMake configure failed", configureResult.Output, sw);

            Console.WriteLine("  Configure: OK");
        }
        else
        {
            Console.WriteLine("[Step 3/6] Skipping CMake configure (cache up-to-date)");
        }

        // [5] CMake build (use mapped CMake config for multi-config generators)
        string cmakeBuildType = MapToCMakeBuildType(config);
        Console.WriteLine($"[Step 4/6] Building ({cmakeBuildType}) ...");
        var buildResult = _cmake.Build(buildDir, cmakeBuildType);
        if (!buildResult.Success)
            return Fail("CMake build failed", buildResult.Output, sw);

        Console.WriteLine("  Build: OK");

        // [6] Post-build step
        Console.WriteLine(isHotReload
            ? "[Step 5/6] Running hot-reload post-build step ..."
            : "[Step 5/6] Running post-build step ...");

        var postBuildContext = new PostBuildContext
        {
            CmakeBuildDir = buildDir,
            ProjectName = scan.ProjectName,
            ScanResult = scan,
            BuildOptions = options,
        };

        var postBuildResult = isHotReload
            ? PostBuildStep.ExecuteHotReload(postBuildContext)
            : PostBuildStep.Execute(postBuildContext);

        if (!postBuildResult.Success)
            return Fail("Post-build failed", postBuildResult.ErrorDetail, sw);

        Console.WriteLine($"  {postBuildResult.Message}");

        // [7] Summary
        sw.Stop();
        PrintSummary(scan, config, platform, sw.Elapsed);

        return BuildResult.Ok(
            $"Build succeeded: {scan.ProjectName} ({config}|{platform}) in {FormatElapsed(sw.Elapsed)}");
    }

    /// <summary>
    /// Generate CMakeLists.txt content via CMakeGenerator.
    /// Uses the engine target (Launch module with main()) for executable determination.
    /// Falls back to game target if no engine target exists (e.g. minimal test projects).
    /// Filters modules to only include those reachable from the project's root modules
    /// (game modules + plugin modules + target entry points).
    /// </summary>
    private static CMakeGenerator.GenerateResult GenerateCMake(
        ProjectScanner.ScanResult scan, BuildConfiguration config, string platform,
        IReadOnlySet<string>? hotReloadModuleNames = null)
    {
        var targetRules = scan.EngineTarget ?? scan.GameTarget;

        // Compute root modules: game + plugin + executable entry points
        var roots = new HashSet<string>(StringComparer.Ordinal);

        foreach (var name in scan.GameModules.Keys)
            roots.Add(name);

        foreach (var name in scan.PluginScanResult.Modules.Keys)
            roots.Add(name);

        if (targetRules is not null)
        {
            foreach (var name in targetRules.ExtraModuleNames)
                roots.Add(name);
        }

        foreach (var name in scan.GameTarget.ExtraModuleNames)
            roots.Add(name);

        // Include DeveloperTool modules as roots (they are loaded at runtime
        // via LoadModule, not referenced by static dependencies).
        // Excluded from Shipping builds. Only engine modules, not ThirdParty.
        if (config != BuildConfiguration.Shipping)
        {
            foreach (var (name, rules) in scan.EngineModules)
            {
                if (rules.Type == ModuleType.DeveloperTool)
                    roots.Add(name);
            }
        }

        // Filter to only reachable modules via dependency graph BFS
        var reachable = DependencyResolver.ComputeReachableSet(
            roots, scan.ResolveResult.AdjacencyList);

        var filteredModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        foreach (var (name, rules) in scan.AllModules)
        {
            if (reachable.Contains(name))
                filteredModules[name] = rules;
        }

        var excluded = scan.AllModules.Keys.Where(k => !reachable.Contains(k)).ToList();
        if (excluded.Count > 0)
        {
            Console.WriteLine($"  Filtered {excluded.Count} unreachable module(s): {string.Join(", ", excluded)}");
        }

        var generator = new CMakeGenerator();
        return generator.Generate(
            scan.ProjectName,
            filteredModules,
            scan.ResolveResult,
            scan.ProjectRoot,
            targetRules,
            config,
            platform,
            hotReloadModuleNames);
    }

    /// <summary>
    /// Determine whether CMake configure needs to run.
    /// Returns true if CMakeCache.txt does not exist or CMakeLists.txt is newer than the cache.
    /// </summary>
    private static bool NeedsConfigure(string buildDir, string cmakeListsPath)
    {
        string cachePath = Path.Combine(buildDir, "CMakeCache.txt");
        if (!File.Exists(cachePath))
            return true;

        // Reconfigure if CMakeLists.txt was regenerated after the cache
        var cacheTime = File.GetLastWriteTimeUtc(cachePath);
        var cmakeTime = File.GetLastWriteTimeUtc(cmakeListsPath);
        return cmakeTime > cacheTime;
    }

    /// <summary>
    /// Run CMake configure with BUILD_SHARED_LIBS and Visual Studio generator.
    /// When hotReloadSuffix is provided, passes HOT_RELOAD_SUFFIX to version PDB output.
    /// </summary>
    private CMakeInvoker.ProcessResult ConfigureCMake(
        string projectRoot, string buildDir, bool isShipping, int? hotReloadSuffix = null)
    {
        var defines = new Dictionary<string, string>
        {
            ["BUILD_SHARED_LIBS:BOOL"] = isShipping ? "OFF" : "ON",
        };

        if (hotReloadSuffix.HasValue)
        {
            defines["HOT_RELOAD_SUFFIX:STRING"] = $"{hotReloadSuffix.Value:D4}";
        }

        return _cmake.Configure(
            projectRoot,
            buildDir,
            "Visual Studio 17 2022",
            defines);
    }

    /// <summary>
    /// Print a formatted build summary.
    /// </summary>
    private static void PrintSummary(
        ProjectScanner.ScanResult scan, BuildConfiguration config, string platform, TimeSpan elapsed)
    {
        Console.WriteLine();
        Console.WriteLine("[Step 6/6] Build Summary");
        Console.WriteLine("┌──────────────────────────────────────────────┐");
        Console.WriteLine($"│ Project:       {scan.ProjectName,-30}│");
        Console.WriteLine($"│ Configuration: {config,-30}│");
        Console.WriteLine($"│ Platform:      {platform,-30}│");
        Console.WriteLine($"│ Modules:       {scan.AllModules.Count,-30}│");
        Console.WriteLine($"│ Elapsed:       {FormatElapsed(elapsed),-30}│");
        Console.WriteLine("└──────────────────────────────────────────────┘");
    }

    /// <summary>
    /// Format elapsed time as human-readable string.
    /// </summary>
    private static string FormatElapsed(TimeSpan elapsed)
    {
        if (elapsed.TotalSeconds < 1)
            return $"{elapsed.TotalMilliseconds:F0}ms";
        if (elapsed.TotalMinutes < 1)
            return $"{elapsed.TotalSeconds:F1}s";
        return $"{elapsed.Minutes}m {elapsed.Seconds}s";
    }

    /// <summary>
    /// Create a failure BuildResult with timing info.
    /// </summary>
    private static BuildResult Fail(string message, string? detail, Stopwatch sw)
    {
        sw.Stop();
        Console.Error.WriteLine($"[BuildPipeline] FAILED: {message}");
        if (detail is not null)
            Console.Error.WriteLine($"  Detail: {detail}");
        Console.Error.WriteLine($"  Elapsed: {FormatElapsed(sw.Elapsed)}");

        return BuildResult.Fail(message, detail);
    }

    /// <summary>
    /// Map EnigmaEngine BuildConfiguration to a standard CMake build type.
    /// Multi-config generators (Visual Studio) only support: Debug, Release, RelWithDebInfo, MinSizeRel.
    ///
    ///   Debug       → Debug       (no optimization, full debug)
    ///   DebugGame   → RelWithDebInfo (partial optimization + debug symbols)
    ///   Development → RelWithDebInfo (partial optimization + debug symbols)
    ///   Shipping    → Release     (full optimization, no debug)
    ///   Test        → Release     (full optimization)
    /// </summary>
    private static string MapToCMakeBuildType(BuildConfiguration config) => config switch
    {
        BuildConfiguration.Debug       => "Debug",
        BuildConfiguration.DebugGame   => "RelWithDebInfo",
        BuildConfiguration.Development => "RelWithDebInfo",
        BuildConfiguration.Shipping    => "Release",
        BuildConfiguration.Test        => "Release",
        _ => "RelWithDebInfo",
    };

    /// <summary>
    /// Write file only if content has changed. Preserves timestamp for incremental cache checks.
    /// Returns true if the file was written (new or changed), false if unchanged.
    /// </summary>
    private static bool WriteIfChanged(string path, string content)
    {
        if (File.Exists(path) && File.ReadAllText(path) == content)
            return false;

        File.WriteAllText(path, content);
        return true;
    }

    /// <summary>
    /// Check if a file is locked by another process (e.g., running engine exe).
    /// </summary>
    private static bool IsFileLocked(string filePath)
    {
        try
        {
            using var stream = File.Open(filePath, FileMode.Open, FileAccess.Write, FileShare.None);
            return false;
        }
        catch (IOException)
        {
            return true;
        }
    }

    /// <summary>
    /// Find a hot-reload suffix whose PDB files are not locked in the intermediate build directory.
    /// When the debugger holds PDBs from a previous session, the linker cannot overwrite them (LNK1201).
    /// This advances the suffix past any locked files.
    /// </summary>
    private static int FindAvailableHotReloadSuffix(string buildDir, int startSuffix)
    {
        if (!Directory.Exists(buildDir))
            return startSuffix;

        const int maxAttempts = 100;
        int suffix = startSuffix;

        for (int i = 0; i < maxAttempts; i++, suffix++)
        {
            string suffixStr = $"-{suffix:D4}.pdb";
            bool anyLocked = false;

            try
            {
                foreach (var pdbFile in Directory.GetFiles(buildDir, "*.pdb", SearchOption.AllDirectories))
                {
                    if (!pdbFile.EndsWith(suffixStr, StringComparison.OrdinalIgnoreCase))
                        continue;

                    if (IsFileLocked(pdbFile))
                    {
                        anyLocked = true;
                        break;
                    }
                }
            }
            catch (DirectoryNotFoundException)
            {
                return suffix;
            }

            if (!anyLocked)
                return suffix;
        }

        return suffix;
    }

    /// <summary>
    /// Collect module names eligible for hot-reload (game modules + project plugin modules).
    /// Engine modules and engine plugins are excluded.
    /// </summary>
    private static HashSet<string> CollectHotReloadModuleNames(ProjectScanner.ScanResult scan)
    {
        var names = new HashSet<string>(StringComparer.Ordinal);

        foreach (var name in scan.GameModules.Keys)
            names.Add(name);

        foreach (var (_, descriptor) in scan.PluginScanResult.EnabledPlugins)
        {
            string pluginDir = Path.GetDirectoryName(descriptor.SourceFilePath)!;
            string normalizedPluginDir = pluginDir.Replace('\\', '/');
            string normalizedEngineRoot = scan.EngineRoot.Replace('\\', '/');
            if (normalizedPluginDir.StartsWith(normalizedEngineRoot, StringComparison.OrdinalIgnoreCase))
                continue;

            foreach (var moduleDesc in descriptor.Modules)
                names.Add(moduleDesc.Name);
        }

        return names;
    }
}
