// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using System.Diagnostics;
using BuildTool.Generators;
using BuildTool.Models;

/// <summary>
/// Core build orchestrator — single entry point for BuildCommand.
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

        // [2] Generate CMakeLists.txt
        Console.WriteLine("[Step 2/6] Generating CMakeLists.txt ...");
        var genResult = GenerateCMake(scan, config, platform);
        if (!genResult.Success)
            return Fail("CMake generation failed", genResult.Error, sw);

        string cmakeListsPath = Path.Combine(scan.ProjectRoot, "CMakeLists.txt");
        bool cmakeChanged = WriteIfChanged(cmakeListsPath, genResult.Content);
        Console.WriteLine(cmakeChanged
            ? $"  Written: {cmakeListsPath}"
            : $"  Unchanged: {cmakeListsPath}");

        // [3] Check CMakeCache.txt — skip reconfigure if cache exists and CMakeLists.txt is older
        bool needsConfigure = NeedsConfigure(buildDir, cmakeListsPath);

        // [4] CMake configure (with BUILD_SHARED_LIBS)
        if (needsConfigure)
        {
            Console.WriteLine("[Step 3/6] Configuring CMake ...");
            var configureResult = ConfigureCMake(scan.ProjectRoot, buildDir, isShipping);
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
        Console.WriteLine("[Step 5/6] Running post-build step ...");
        string outputDir = Path.Combine(scan.ProjectRoot, "Binaries", platform);
        var postBuildResult = PostBuildStep.Execute(new PostBuildContext
        {
            CmakeBuildDir = buildDir,
            OutputDir = outputDir,
            ProjectName = scan.ProjectName,
            ScanResult = scan,
            BuildOptions = options,
        });

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
    /// </summary>
    private static CMakeGenerator.GenerateResult GenerateCMake(
        ProjectScanner.ScanResult scan, BuildConfiguration config, string platform)
    {
        var generator = new CMakeGenerator();
        return generator.Generate(
            scan.ProjectName,
            scan.AllModules,
            scan.ResolveResult,
            scan.ProjectRoot,
            scan.EngineTarget ?? scan.GameTarget,
            config,
            platform);
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
    /// </summary>
    private CMakeInvoker.ProcessResult ConfigureCMake(
        string projectRoot, string buildDir, bool isShipping)
    {
        var defines = new Dictionary<string, string>
        {
            ["BUILD_SHARED_LIBS:BOOL"] = isShipping ? "OFF" : "ON",
        };

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
}
