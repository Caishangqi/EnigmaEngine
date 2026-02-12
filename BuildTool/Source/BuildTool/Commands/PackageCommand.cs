// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Build;
using BuildTool.Generators;
using BuildTool.Models;

namespace BuildTool.Commands;

/// <summary>
/// Builds the project with Shipping configuration (monolithic),
/// then executes the PackagePlan to create a UE-style shipped directory structure.
///
/// Usage: BuildTool package &lt;project&gt; [-o|--output &lt;path&gt;] [-c|--config Shipping]
///
/// Output directory priority:
///   1. CLI --output flag
///   2. .eproject "StagingDirectory" field
///   3. Default: {ProjectRoot}/Saved/StagedBuilds/{Platform}/
/// </summary>
public sealed class PackageCommand : ICommand
{
    private readonly BuildCommand _buildCommand = new();

    public string Name => "package";
    public string Description => "Build and package the project for distribution.";

    public BuildResult Execute(BuildOptions options)
    {
        try
        {
            Console.WriteLine("╔══════════════════════════════════════════════╗");
            Console.WriteLine("║       EnigmaEngine Package Tool              ║");
            Console.WriteLine("╚══════════════════════════════════════════════╝");
            Console.WriteLine($"  Project: {options.ProjectPath}");
            Console.WriteLine($"  Platform: {options.Platform}");
            Console.WriteLine();

            // 1. Force Shipping configuration (package = production distribution)
            var shippingOptions = new BuildOptions
            {
                ProjectPath = options.ProjectPath,
                Configuration = BuildConfiguration.Shipping,
                Platform = options.Platform,
                OutputDirectory = options.OutputDirectory,
            };

            // 2. Build with Shipping config
            Console.WriteLine("[Package] Step 1/3: Building (Shipping) ...");
            var buildResult = _buildCommand.Execute(shippingOptions);
            if (!buildResult.Success)
                return BuildResult.Fail("Package aborted: build step failed.", buildResult.ErrorDetail);

            Console.WriteLine();
            Console.WriteLine("[Package] Step 2/3: Generating package plan ...");

            // 3. Scan project for module info
            var scan = ProjectScanner.Scan(options.ProjectPath);

            // 4. Generate PackagePlan via ShippingPackager
            var packager = new ShippingPackager();
            var engineModuleNames = new HashSet<string>(
                scan.EngineModules.Keys, StringComparer.Ordinal);

            var plan = packager.GeneratePlan(
                scan.ProjectName,
                scan.AllModules,
                scan.GameTarget,
                shippingOptions.Platform,
                scan.PluginScanResult,
                engineModuleNames);

            if (!plan.Success)
                return BuildResult.Fail("Package plan generation failed", plan.Error);

            // 5. Resolve output directory (3-level priority)
            string outputRoot = ResolveOutputDirectory(
                options.OutputDirectory,
                scan.ProjectDescriptor.StagingDirectory,
                scan.ProjectRoot,
                options.Platform);

            Console.WriteLine($"[Package] Output: {outputRoot}");
            Console.WriteLine();
            Console.WriteLine($"[Package] Step 3/3: Copying {plan.Entries.Count} entries ...");

            // 6. Execute the plan
            var packageResult = PackageExecutor.Execute(plan, outputRoot, scan.ProjectRoot);
            if (!packageResult.Success)
                return BuildResult.Fail("Packaging failed", packageResult.Error);

            return BuildResult.Ok(
                $"Package succeeded: {packageResult.FilesCopied} copied, " +
                $"{packageResult.FilesGenerated} generated → {outputRoot}");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Package failed with unexpected error", ex.Message);
        }
    }

    /// <summary>
    /// Resolve the output directory using three-level priority:
    ///   1. CLI --output flag
    ///   2. .eproject StagingDirectory field
    ///   3. Default: {ProjectRoot}/Saved/StagedBuilds/{Platform}/
    /// </summary>
    internal static string ResolveOutputDirectory(
        string? cliOutput,
        string? eprojectStagingDir,
        string projectRoot,
        string platform)
    {
        if (!string.IsNullOrWhiteSpace(cliOutput))
        {
            string resolved = Path.GetFullPath(cliOutput);
            return Path.Combine(resolved, platform);
        }

        if (!string.IsNullOrWhiteSpace(eprojectStagingDir))
        {
            string resolved = Path.IsPathRooted(eprojectStagingDir)
                ? eprojectStagingDir
                : Path.GetFullPath(Path.Combine(projectRoot, eprojectStagingDir));
            return Path.Combine(resolved, platform);
        }

        return Path.Combine(projectRoot, "Saved", "StagedBuilds", platform);
    }
}