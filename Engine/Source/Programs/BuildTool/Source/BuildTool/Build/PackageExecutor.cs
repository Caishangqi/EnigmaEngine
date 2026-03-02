// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Build;

using BuildTool.Generators;

/// <summary>
/// Executes a <see cref="ShippingPackager.PackagePlan"/> by copying source files
/// and writing generated content to the target output directory.
///
/// Pure I/O executor - all structural logic lives in the plan.
/// Follows the same static-class pattern as <see cref="PostBuildStep"/>.
/// </summary>
public static class PackageExecutor
{
    /// <summary>
    /// Result of package execution.
    /// </summary>
    public sealed class PackageResult
    {
        public required bool Success { get; init; }
        public string? Error { get; init; }
        public int FilesCopied { get; init; }
        public int FilesGenerated { get; init; }

        public static PackageResult Ok(int copied, int generated) =>
            new() { Success = true, FilesCopied = copied, FilesGenerated = generated };

        public static PackageResult Fail(string error) =>
            new() { Success = false, Error = error };
    }

    /// <summary>
    /// Execute the package plan: copy/generate all entries to <paramref name="outputRoot"/>.
    /// </summary>
    /// <param name="plan">The packaging plan from <see cref="ShippingPackager.GeneratePlan"/>.</param>
    /// <param name="outputRoot">Absolute path to the output root directory.</param>
    /// <param name="projectRoot">Absolute path to the project root (resolves relative SourcePaths).</param>
    public static PackageResult Execute(
        ShippingPackager.PackagePlan plan,
        string outputRoot,
        string projectRoot)
    {
        if (!plan.Success)
            return PackageResult.Fail(plan.Error ?? "Package plan was not successful.");

        try
        {
            // Clean output directory for a fresh staged build
            if (Directory.Exists(outputRoot))
            {
                Console.WriteLine($"[Package] Cleaning existing output: {outputRoot}");
                Directory.Delete(outputRoot, recursive: true);
            }

            Directory.CreateDirectory(outputRoot);

            int filesCopied = 0;
            int filesGenerated = 0;

            foreach (var entry in plan.Entries)
            {
                string destPath = Path.Combine(outputRoot, entry.DestinationPath.Replace('/', Path.DirectorySeparatorChar));
                string? destDir = Path.GetDirectoryName(destPath);
                if (destDir is not null)
                    Directory.CreateDirectory(destDir);

                if (entry.GeneratedContent is not null)
                {
                    File.WriteAllText(destPath, entry.GeneratedContent);
                    Console.WriteLine($"  [Generate] {entry.DestinationPath}");
                    filesGenerated++;
                }
                else if (entry.SourcePath is not null)
                {
                    string sourcePath = Path.IsPathRooted(entry.SourcePath)
                        ? entry.SourcePath
                        : Path.Combine(projectRoot, entry.SourcePath.Replace('/', Path.DirectorySeparatorChar));

                    if (File.Exists(sourcePath))
                    {
                        File.Copy(sourcePath, destPath, overwrite: true);
                        Console.WriteLine($"  [Copy] {entry.DestinationPath}");
                        filesCopied++;
                    }
                    else
                    {
                        Console.WriteLine($"  [Skip] {entry.DestinationPath} - source not found: {sourcePath}");
                    }
                }
            }

            // Generate Manifest_NonUFSFiles_Win64.txt (list of all physical files)
            GenerateNonUfsManifest(outputRoot);

            Console.WriteLine($"[Package] Complete: {filesCopied} copied, {filesGenerated} generated → {outputRoot}");
            return PackageResult.Ok(filesCopied, filesGenerated);
        }
        catch (Exception ex)
        {
            return PackageResult.Fail($"Packaging failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Generate Manifest_NonUFSFiles_Win64.txt listing all physical files in the output directory.
    /// Matches UE's manifest format: one relative path per line.
    /// </summary>
    private static void GenerateNonUfsManifest(string outputRoot)
    {
        var manifestPath = Path.Combine(outputRoot, "Manifest_NonUFSFiles_Win64.txt");
        var files = Directory.GetFiles(outputRoot, "*", SearchOption.AllDirectories)
            .Select(f => Path.GetRelativePath(outputRoot, f).Replace('\\', '/'))
            .Where(f => !f.StartsWith("Manifest_", StringComparison.Ordinal))
            .OrderBy(f => f, StringComparer.Ordinal)
            .ToList();

        File.WriteAllLines(manifestPath, files);
        Console.WriteLine($"  [Generate] Manifest_NonUFSFiles_Win64.txt ({files.Count} entries)");
    }
}
