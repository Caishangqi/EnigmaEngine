// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scaffolding;

/// <summary>
/// Removes a plugin from the project: checks dependencies, updates .eproject,
/// deletes the plugin directory, and regenerates project files.
/// Does NOT modify .Target.cs - plugins are discovered via .eplugin descriptors.
/// </summary>
public sealed class RemovePluginCommand : ICommand
{
    public string Name => "remove-plugin";
    public string Description => "Remove a plugin from an existing project.";

    public BuildResult Execute(BuildOptions options)
    {
        // 1. Parse arguments
        if (!options.ExtraArguments.TryGetValue("name", out var pluginName) || string.IsNullOrWhiteSpace(pluginName))
            return BuildResult.Fail("--name is required. Usage: remove-plugin --project <path> --name <PluginName>");

        var force = options.ExtraArguments.ContainsKey("force");

        try
        {
            // 2. Scan project
            Console.WriteLine("[RemovePlugin] Scanning project...");
            var scan = ProjectScanner.Scan(options.ProjectPath);

            // 3. Verify plugin exists
            if (!scan.PluginScanResult.EnabledPlugins.ContainsKey(pluginName))
                return BuildResult.Fail($"Plugin '{pluginName}' not found in the project.");

            // 4. Dependency check (external modules depending on plugin's modules)
            Console.WriteLine("[RemovePlugin] Checking dependencies...");
            var depCheck = DependencyChecker.FindPluginDependents(
                pluginName, scan.PluginScanResult, scan.ResolveResult);
            if (!depCheck.IsSafeToRemove)
            {
                var depList = string.Join(", ", depCheck.Dependents);
                if (!force)
                    return BuildResult.Fail($"Plugin '{pluginName}' has dependents: [{depList}]. Use --force to override.");

                Console.WriteLine($"[RemovePlugin] Warning: Forcing removal despite dependents: [{depList}]");
            }

            // 5. Update .eproject (plugin only - no .Target.cs modification)
            Console.WriteLine("[RemovePlugin] Updating .eproject...");
            var eprojectResult = EprojectModifier.RemovePlugin(scan.EprojectPath, pluginName);
            if (!eprojectResult.Success)
                return BuildResult.Fail(eprojectResult.Message);

            // 6. Delete plugin directory
            var pluginDir = Path.Combine(scan.ProjectRoot, "Plugins", pluginName);
            if (Directory.Exists(pluginDir))
            {
                Console.WriteLine($"[RemovePlugin] Deleting {pluginDir}...");
                Directory.Delete(pluginDir, recursive: true);
            }

            // 7. Regenerate project files (warn on failure)
            Console.WriteLine("[RemovePlugin] Regenerating project files...");
            var genResult = new GenerateProjectFilesCommand().Execute(options);
            if (!genResult.Success)
                Console.WriteLine($"[RemovePlugin] Warning: Project file regeneration failed: {genResult.Message}");

            // 8. Summary
            Console.WriteLine();
            Console.WriteLine($"Plugin '{pluginName}' removed successfully.");

            return BuildResult.Ok($"Plugin '{pluginName}' removed successfully.");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Plugin removal failed.", ex.Message);
        }
    }
}
