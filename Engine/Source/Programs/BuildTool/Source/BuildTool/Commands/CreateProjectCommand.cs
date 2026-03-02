// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scaffolding;

/// <summary>
/// Creates a new game project from templates. Generates all project files including
/// .eproject, .Target.cs, game module, and GameInstance. Does NOT call EprojectModifier
/// or TargetFileModifier - these files come directly from templates.
/// Supports <c>--template</c> to select a variant and <c>--list-templates</c> to
/// print all discovered templates.
/// </summary>
public sealed class CreateProjectCommand : ICommand
{
    public string Name => "create-project";
    public string Description => "Create a new game project.";

    public BuildResult Execute(BuildOptions options)
    {
        // 1. Handle --list-templates (early exit)
        if (options.ExtraArguments.ContainsKey("list-templates"))
            return ListTemplates();

        // 2. Parse arguments
        if (!options.ExtraArguments.TryGetValue("name", out var projectName) || string.IsNullOrWhiteSpace(projectName))
            return BuildResult.Fail("--name is required. Usage: create-project --name <ProjectName> --location <path> [--template <VariantName>]");

        var location = options.ExtraArguments.GetValueOrDefault("location", ".");
        var templateName = options.ExtraArguments.GetValueOrDefault("template", "Default");

        // Guard against empty --template value (boolean-flag parse)
        if (string.IsNullOrWhiteSpace(templateName))
            templateName = "Default";

        try
        {
            // 3. Resolve engine root
            Console.WriteLine("[CreateProject] Resolving engine root...");
            var engineRoot = FindEngineRoot();
            var repoRoot = Path.GetDirectoryName(engineRoot)!;

            // 4. Resolve template
            Console.WriteLine($"[CreateProject] Resolving template '{templateName}'...");
            var templateInfo = ProjectTemplateDiscovery.Find(repoRoot, templateName);
            if (templateInfo is null)
                return TemplateNotFoundError(repoRoot, templateName);

            // 5. Validate name (empty ExistingNames - new project has no conflicts)
            Console.WriteLine($"[CreateProject] Validating name '{projectName}'...");
            var context = ValidationContext.Create(
                engineModuleNames: [],
                existingNames: []);
            var validation = NameValidator.Validate(projectName, context);
            if (!validation.Success)
                return BuildResult.Fail(validation.Message);

            // 6. Check directory conflict
            if (!Directory.Exists(location))
                Directory.CreateDirectory(location);
            var projectDir = Path.GetFullPath(Path.Combine(location, projectName));
            if (Directory.Exists(projectDir))
                return BuildResult.Fail($"Directory already exists: {projectDir}");

            // 7. Compute ENGINE_ROOT_RELATIVE
            var engineRootRelative = Path.GetRelativePath(projectDir, engineRoot)
                .Replace('\\', '/');
            if (!engineRootRelative.EndsWith('/'))
                engineRootRelative += '/';

            // 8. Build replacements
            var replacements = new Dictionary<string, string>
            {
                ["PROJECT_NAME_UPPER"] = projectName.ToUpperInvariant(),
                ["PROJECT_NAME_API"] = projectName.ToUpperInvariant() + "_API",
                ["PROJECT_NAME"] = projectName,
                ["ENGINE_ROOT_RELATIVE"] = engineRootRelative,
                ["TEMPLATE"] = templateInfo.Name,
            };

            // 9. Scaffold with rollback protection
            var templateDir = templateInfo.DirectoryPath;
            using var rollback = new ScaffoldingRollback();

            Console.WriteLine($"[CreateProject] Processing templates from '{templateInfo.Name}'...");
            var engine = new TemplateEngine();
            var processResult = engine.Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = projectDir,
                Replacements = replacements,
            });

            if (!processResult.Success)
                return BuildResult.Fail($"Template processing failed: {processResult.Error}");

            // Track created artifacts for rollback
            foreach (var file in processResult.CreatedFiles)
                rollback.TrackFile(file);
            foreach (var dir in processResult.CreatedDirectories)
                rollback.TrackDirectory(dir);
            rollback.TrackDirectory(projectDir);

            // 10. Commit - prevents rollback on dispose
            rollback.Commit();
            Console.WriteLine("[CreateProject] Changes committed.");

            // 11. Regenerate project files (warn on failure, don't rollback)
            Console.WriteLine("[CreateProject] Regenerating project files...");
            var genOptions = new BuildOptions
            {
                ProjectPath = projectDir,
                ExtraArguments = options.ExtraArguments,
            };
            var genResult = new GenerateProjectFilesCommand().Execute(genOptions);
            if (!genResult.Success)
                Console.WriteLine($"[CreateProject] Warning: Project file regeneration failed: {genResult.Message}");

            // 12. Summary
            Console.WriteLine();
            Console.WriteLine($"Project '{projectName}' created successfully:");
            Console.WriteLine($"  Template:  {templateInfo.DisplayName} ({templateInfo.Name})");
            Console.WriteLine($"  Directory: {projectDir}");
            Console.WriteLine($"  Files:     {processResult.CreatedFiles.Count}");
            Console.WriteLine();
            Console.WriteLine($"Next steps:");
            Console.WriteLine($"  1. Open {projectName}.sln in your IDE");
            Console.WriteLine($"  2. Build and run!");

            return BuildResult.Ok($"Project '{projectName}' created successfully.");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Project creation failed.", ex.Message);
        }
    }

    /// <summary>
    /// Lists all available project templates and returns success.
    /// </summary>
    private static BuildResult ListTemplates()
    {
        try
        {
            var engineRoot = FindEngineRoot();
            var repoRoot = Path.GetDirectoryName(engineRoot)!;
            var templates = ProjectTemplateDiscovery.Discover(repoRoot);

            if (templates.Count == 0)
            {
                Console.WriteLine("No project templates found.");
                return BuildResult.Ok("No templates found.");
            }

            Console.WriteLine("Available project templates:");
            Console.WriteLine();
            foreach (var t in templates)
            {
                var suffix = string.Equals(t.Name, "Default", StringComparison.OrdinalIgnoreCase)
                    ? " (default)"
                    : "";
                Console.WriteLine($"  {t.Name,-20} {t.DisplayName}{suffix}");
                if (!string.IsNullOrWhiteSpace(t.Description))
                    Console.WriteLine($"  {"",-20} {t.Description}");
            }

            return BuildResult.Ok("Templates listed.");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Failed to list templates.", ex.Message);
        }
    }

    /// <summary>
    /// Returns an error with available templates when the requested template is not found.
    /// </summary>
    private static BuildResult TemplateNotFoundError(string repoRoot, string templateName)
    {
        var templates = ProjectTemplateDiscovery.Discover(repoRoot);
        var available = templates.Count > 0
            ? string.Join(", ", templates.Select(t => t.Name))
            : "(none)";
        return BuildResult.Fail(
            $"Template '{templateName}' not found. Available templates: {available}");
    }

    /// <summary>
    /// Resolve engine root by walking up from <see cref="AppContext.BaseDirectory"/>
    /// until <c>Engine/Templates/</c> is found.
    /// </summary>
    internal static string FindEngineRoot()
    {
        var current = AppContext.BaseDirectory;
        for (int i = 0; i < 10; i++)
        {
            var parent = Path.GetDirectoryName(current);
            if (parent is null || parent == current)
                break;

            var engineCandidate = Path.Combine(parent, "Engine");
            if (Directory.Exists(Path.Combine(engineCandidate, "Templates")))
                return engineCandidate;

            current = parent;
        }

        throw new DirectoryNotFoundException(
            $"Engine root not found. Expected Engine/Templates/ above {AppContext.BaseDirectory}");
    }
}