// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scaffolding;

/// <summary>
/// Creates a new plugin in the project: generates source files from templates
/// and registers the plugin in .eproject. Does NOT modify .Target.cs - plugins
/// are discovered via .eplugin descriptors.
/// </summary>
public sealed class CreatePluginCommand : ICommand
{
    public string Name => "create-plugin";
    public string Description => "Create a new plugin in an existing project.";

    public BuildResult Execute(BuildOptions options)
    {
        // 1. Parse arguments
        if (!options.ExtraArguments.TryGetValue("name", out var pluginName) || string.IsNullOrWhiteSpace(pluginName))
            return BuildResult.Fail("--name is required. Usage: create-plugin --project <path> --name <PluginName>");

        var category = options.ExtraArguments.GetValueOrDefault("category", "Gameplay");

        try
        {
            // 2. Scan project
            Console.WriteLine("[CreatePlugin] Scanning project...");
            var scan = ProjectScanner.Scan(options.ProjectPath);

            // 3. Validate name
            Console.WriteLine($"[CreatePlugin] Validating name '{pluginName}'...");
            var context = ValidationContext.Create(
                scan.EngineModules.Keys,
                scan.GameModules.Keys);
            var validation = NameValidator.Validate(pluginName, context);
            if (!validation.Success)
                return BuildResult.Fail(validation.Message);

            // 4. Check directory conflict
            var pluginDir = Path.Combine(scan.ProjectRoot, "Plugins", pluginName);
            if (Directory.Exists(pluginDir))
                return BuildResult.Fail($"Directory already exists: {pluginDir}");

            // 5. Build replacements
            var replacements = new Dictionary<string, string>
            {
                ["PLUGIN_NAME_UPPER"] = pluginName.ToUpperInvariant(),
                ["PLUGIN_NAME_API"] = pluginName.ToUpperInvariant() + "_API",
                ["PLUGIN_NAME"] = pluginName,
                ["PLUGIN_CATEGORY"] = category,
            };

            // 6. Scaffold with rollback protection
            var templateDir = Path.Combine(scan.EngineRoot, "Templates", "Plugin");
            using var rollback = new ScaffoldingRollback();

            Console.WriteLine("[CreatePlugin] Processing templates...");
            var engine = new TemplateEngine();
            var processResult = engine.Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = pluginDir,
                Replacements = replacements,
            });

            if (!processResult.Success)
                return BuildResult.Fail($"Template processing failed: {processResult.Error}");

            // Track created artifacts for rollback
            foreach (var file in processResult.CreatedFiles)
                rollback.TrackFile(file);
            foreach (var dir in processResult.CreatedDirectories)
                rollback.TrackDirectory(dir);
            rollback.TrackDirectory(pluginDir);

            // 7. Update .eproject (plugin only - no .Target.cs modification)
            Console.WriteLine("[CreatePlugin] Updating .eproject...");
            rollback.TrackModifiedFile(scan.EprojectPath);
            var eprojectResult = EprojectModifier.AddPlugin(scan.EprojectPath, pluginName);
            if (!eprojectResult.Success)
                return BuildResult.Fail(eprojectResult.Message);

            // 8. Commit - prevents rollback on dispose
            rollback.Commit();
            Console.WriteLine("[CreatePlugin] Changes committed.");

            // 9. Regenerate project files (warn on failure, don't rollback)
            Console.WriteLine("[CreatePlugin] Regenerating project files...");
            var genResult = new GenerateProjectFilesCommand().Execute(options);
            if (!genResult.Success)
                Console.WriteLine($"[CreatePlugin] Warning: Project file regeneration failed: {genResult.Message}");

            // 10. Summary
            Console.WriteLine();
            Console.WriteLine($"Plugin '{pluginName}' created successfully:");
            Console.WriteLine($"  Directory: {pluginDir}");
            Console.WriteLine($"  Files:     {processResult.CreatedFiles.Count}");
            Console.WriteLine($"  Category:  {category}");

            return BuildResult.Ok($"Plugin '{pluginName}' created successfully.");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Plugin creation failed.", ex.Message);
        }
    }
}
