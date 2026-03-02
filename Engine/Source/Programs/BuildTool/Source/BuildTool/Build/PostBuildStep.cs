// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using BuildTool.Generators;
using BuildTool.Models;

/// <summary>
/// Context for post-build binary placement and manifest generation.
/// </summary>
public sealed class PostBuildContext
{
    /// <summary>CMake build output directory (contains compiled binaries).</summary>
    public required string CmakeBuildDir { get; init; }

    /// <summary>Target output directory (e.g., {ProjectRoot}/Binaries/Win64/).</summary>
    public required string OutputDir { get; init; }

    /// <summary>Project name used in binary naming.</summary>
    public required string ProjectName { get; init; }

    /// <summary>Full project scan result.</summary>
    public required ProjectScanner.ScanResult ScanResult { get; init; }

    /// <summary>Build options (configuration, platform).</summary>
    public required BuildOptions BuildOptions { get; init; }
}

/// <summary>
/// Post-build step: copies compiled binaries to the output directory and generates manifest files.
///
/// Modular (Development/DebugGame/Debug/Test):
///   - Copies all .exe/.dll/.pdb flat to Binaries/Win64/
///   - Generates .modules and .target manifests
///
/// Monolithic (Shipping):
///   - Copies only the monolithic .exe and .pdb (no module DLLs)
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
        bool isMonolithic = config == BuildConfiguration.Shipping;
        string linkType = isMonolithic ? "Monolithic" : "Modular";

        Console.WriteLine($"[PostBuild] Mode: {linkType} ({config})");
        Console.WriteLine($"[PostBuild] Source: {context.CmakeBuildDir}");
        Console.WriteLine($"[PostBuild] Output: {context.OutputDir}");

        try
        {
            // 1. Ensure output directory exists
            Directory.CreateDirectory(context.OutputDir);

            // 2. Copy binaries from CMake build dir to output dir
            int copied = isMonolithic
                ? CopyMonolithicBinaries(context.CmakeBuildDir, context.OutputDir)
                : CopyModularBinaries(context.CmakeBuildDir, context.OutputDir);

            Console.WriteLine($"[PostBuild] Copied {copied} file(s)");

            // 3. Generate manifests via ManifestGenerator
            var manifestResult = GenerateManifests(context, linkType);
            if (!manifestResult.Success)
                return BuildResult.Fail("Manifest generation failed", manifestResult.Error);

            // 4. Write manifest files to disk
            foreach (var (relativePath, content) in manifestResult.Files)
            {
                string fullPath = Path.Combine(context.ScanResult.ProjectRoot, relativePath);
                string? dir = Path.GetDirectoryName(fullPath);
                if (dir is not null)
                    Directory.CreateDirectory(dir);

                File.WriteAllText(fullPath, content);
                Console.WriteLine($"[PostBuild] Manifest: {relativePath}");
            }

            return BuildResult.Ok(
                $"Post-build completed: {copied} binaries, {manifestResult.Files.Count} manifests ({linkType})");
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
    /// Copy all .exe, .dll, .pdb files flat from build directory to output directory.
    /// Used for Modular builds (Development/DebugGame/Debug/Test).
    /// </summary>
    private static int CopyModularBinaries(string buildDir, string outputDir)
    {
        int count = 0;
        foreach (var pattern in new[] { "*.exe", "*.dll", "*.pdb" })
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
    /// </summary>
    private static ManifestGenerator.GenerateResult GenerateManifests(
        PostBuildContext context, string linkType)
    {
        var generator = new ManifestGenerator();
        return generator.Generate(
            context.ProjectName,
            context.ScanResult.AllModules,
            context.BuildOptions.Configuration,
            context.BuildOptions.Platform,
            context.ScanResult.GameTarget,
            context.ScanResult.PluginScanResult,
            linkType: linkType);
    }
}
