// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scaffolding;

/// <summary>
/// Removes a module from the project: checks dependencies, updates .eproject and .Target.cs,
/// deletes the source directory, and regenerates project files.
/// </summary>
public sealed class RemoveModuleCommand : ICommand
{
    public string Name => "remove-module";
    public string Description => "Remove a module from an existing project.";

    /// <summary>Engine modules that cannot be removed.</summary>
    private static readonly HashSet<string> ProtectedEngineModules =
        new(StringComparer.OrdinalIgnoreCase) { "Core", "Engine", "Launch" };

    public BuildResult Execute(BuildOptions options)
    {
        // 1. Parse arguments
        if (!options.ExtraArguments.TryGetValue("name", out var moduleName) || string.IsNullOrWhiteSpace(moduleName))
            return BuildResult.Fail("--name is required. Usage: remove-module --project <path> --name <ModuleName>");

        var force = options.ExtraArguments.ContainsKey("force");

        try
        {
            // 2. Scan project
            Console.WriteLine("[RemoveModule] Scanning project...");
            var scan = ProjectScanner.Scan(options.ProjectPath);

            // 3. Verify module exists
            if (!scan.GameModules.ContainsKey(moduleName) &&
                !scan.EngineModules.ContainsKey(moduleName))
                return BuildResult.Fail($"Module '{moduleName}' not found in the project.");

            // 4. Primary module protection
            var projectName = scan.ProjectName;
            if (string.Equals(moduleName, projectName, StringComparison.OrdinalIgnoreCase))
                return BuildResult.Fail($"Cannot remove primary module '{moduleName}'. It is the project's main module.");

            // 5. Engine module protection
            if (ProtectedEngineModules.Contains(moduleName))
                return BuildResult.Fail($"Cannot remove engine module '{moduleName}'.");

            // 6. Dependency check
            Console.WriteLine("[RemoveModule] Checking dependencies...");
            var depCheck = DependencyChecker.FindDependents(moduleName, scan.ResolveResult);
            if (!depCheck.IsSafeToRemove)
            {
                var depList = string.Join(", ", depCheck.Dependents);
                if (!force)
                    return BuildResult.Fail($"Module '{moduleName}' has dependents: [{depList}]. Use --force to override.");

                Console.WriteLine($"[RemoveModule] Warning: Forcing removal despite dependents: [{depList}]");
            }

            // 7. Update .eproject
            Console.WriteLine("[RemoveModule] Updating .eproject...");
            var eprojectResult = EprojectModifier.RemoveModule(scan.EprojectPath, moduleName);
            if (!eprojectResult.Success)
                return BuildResult.Fail(eprojectResult.Message);

            // 8. Update .Target.cs
            Console.WriteLine("[RemoveModule] Updating .Target.cs...");
            var targetResult = TargetFileModifier.RemoveModule(scan.GameTarget.SourceFilePath, moduleName);
            if (!targetResult.Success)
                return BuildResult.Fail(targetResult.Message);

            // 9. Delete source directory
            var moduleDir = Path.Combine(scan.ProjectRoot, "Source", moduleName);
            if (Directory.Exists(moduleDir))
            {
                Console.WriteLine($"[RemoveModule] Deleting {moduleDir}...");
                Directory.Delete(moduleDir, recursive: true);
            }

            // 10. Regenerate project files (warn on failure)
            Console.WriteLine("[RemoveModule] Regenerating project files...");
            var genResult = new GenerateProjectFilesCommand().Execute(options);
            if (!genResult.Success)
                Console.WriteLine($"[RemoveModule] Warning: Project file regeneration failed: {genResult.Message}");

            // 11. Summary
            Console.WriteLine();
            Console.WriteLine($"Module '{moduleName}' removed successfully.");

            return BuildResult.Ok($"Module '{moduleName}' removed successfully.");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Module removal failed.", ex.Message);
        }
    }
}
