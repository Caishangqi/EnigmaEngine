using BuildTool.Build;
using BuildTool.Models;

namespace BuildTool.Commands;

/// <summary>
/// Executes a full build pipeline:
/// ProjectScanner.Scan() → BuildPipeline.Run() → BuildResult.
/// Delegates all heavy lifting to ProjectScanner and BuildPipeline.
/// </summary>
public sealed class BuildCommand : ICommand
{
    public string Name => "build";
    public string Description => "Build the specified project.";

    public BuildResult Execute(BuildOptions options)
    {
        try
        {
            Console.WriteLine("╔══════════════════════════════════════════════╗");
            Console.WriteLine("║         EnigmaEngine BuildTool               ║");
            Console.WriteLine("╚══════════════════════════════════════════════╝");
            Console.WriteLine($"  Project:       {options.ProjectPath}");
            Console.WriteLine($"  Configuration: {options.Configuration}");
            Console.WriteLine($"  Platform:      {options.Platform}");
            Console.WriteLine();

            // 1. Scan project: parse descriptors, targets, modules, resolve dependencies
            Console.WriteLine("[Build] Scanning project ...");
            var scanResult = ProjectScanner.Scan(options.ProjectPath);
            Console.WriteLine($"[Build] Found {scanResult.AllModules.Count} modules");
            Console.WriteLine();

            // 2. Auto-regenerate project files if vcxproj missing
            string projectFilesDir = Path.Combine(scanResult.ProjectRoot, "Intermediate", "ProjectFiles");
            bool vcxprojMissing = !Directory.Exists(projectFilesDir)
                || Directory.GetFiles(projectFilesDir, "*.vcxproj").Length == 0;

            if (vcxprojMissing)
            {
                Console.WriteLine("[Build] Project files missing, regenerating...");
                var genCommand = new GenerateProjectFilesCommand();
                var genResult = genCommand.Execute(options);
                if (!genResult.Success)
                    return genResult;
                Console.WriteLine();
            }

            // 3. Run build pipeline: generate CMake → configure → build → post-build
            var invoker = new CMakeInvoker();
            var pipeline = new BuildPipeline(invoker);
            return pipeline.Run(scanResult, options);
        }
        catch (FileNotFoundException ex)
        {
            return BuildResult.Fail("Project file not found", ex.Message);
        }
        catch (DirectoryNotFoundException ex)
        {
            return BuildResult.Fail("Required directory not found", ex.Message);
        }
        catch (InvalidOperationException ex)
        {
            return BuildResult.Fail("Project configuration error", ex.Message);
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Build failed with unexpected error", ex.Message);
        }
    }
}
