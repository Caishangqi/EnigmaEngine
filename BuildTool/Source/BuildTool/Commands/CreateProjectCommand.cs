// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scaffolding;

/// <summary>
/// Creates a new game project from templates. Generates all project files including
/// .eproject, .Target.cs, game module, and GameInstance. Does NOT call EprojectModifier
/// or TargetFileModifier — these files come directly from templates.
/// </summary>
public sealed class CreateProjectCommand : ICommand
{
    public string Name => "create-project";
    public string Description => "Create a new game project.";

    public BuildResult Execute(BuildOptions options)
    {
        // 1. Parse arguments
        if (!options.ExtraArguments.TryGetValue("name", out var projectName) || string.IsNullOrWhiteSpace(projectName))
            return BuildResult.Fail("--name is required. Usage: create-project --name <ProjectName> --location <path>");

        var location = options.ExtraArguments.GetValueOrDefault("location", ".");

        try
        {
            // 2. Resolve engine root
            Console.WriteLine("[CreateProject] Resolving engine root...");
            var engineRoot = FindEngineRoot();

            // 3. Validate name (empty ExistingNames — new project has no conflicts)
            Console.WriteLine($"[CreateProject] Validating name '{projectName}'...");
            var context = ValidationContext.Create(
                engineModuleNames: [],
                existingNames: []);
            var validation = NameValidator.Validate(projectName, context);
            if (!validation.Success)
                return BuildResult.Fail(validation.Message);

            // 4. Check directory conflict
            if (!Directory.Exists(location))
                Directory.CreateDirectory(location);
            var projectDir = Path.GetFullPath(Path.Combine(location, projectName));
            if (Directory.Exists(projectDir))
                return BuildResult.Fail($"Directory already exists: {projectDir}");

            // 5. Compute ENGINE_ROOT_RELATIVE
            var engineRootRelative = Path.GetRelativePath(projectDir, engineRoot)
                .Replace('\\', '/');
            if (!engineRootRelative.EndsWith('/'))
                engineRootRelative += '/';

            // 6. Build replacements
            var replacements = new Dictionary<string, string>
            {
                ["PROJECT_NAME_UPPER"] = projectName.ToUpperInvariant(),
                ["PROJECT_NAME_API"] = projectName.ToUpperInvariant() + "_API",
                ["PROJECT_NAME"] = projectName,
                ["ENGINE_ROOT_RELATIVE"] = engineRootRelative,
            };

            // 7. Scaffold with rollback protection
            var templateDir = Path.Combine(engineRoot, "Templates", "Project");
            using var rollback = new ScaffoldingRollback();

            Console.WriteLine("[CreateProject] Processing templates...");
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

            // 8. Commit — prevents rollback on dispose
            rollback.Commit();
            Console.WriteLine("[CreateProject] Changes committed.");

            // 9. Regenerate project files (warn on failure, don't rollback)
            Console.WriteLine("[CreateProject] Regenerating project files...");
            var genOptions = new BuildOptions
            {
                ProjectPath = projectDir,
                ExtraArguments = options.ExtraArguments,
            };
            var genResult = new GenerateProjectFilesCommand().Execute(genOptions);
            if (!genResult.Success)
                Console.WriteLine($"[CreateProject] Warning: Project file regeneration failed: {genResult.Message}");

            // 10. Summary
            Console.WriteLine();
            Console.WriteLine($"Project '{projectName}' created successfully:");
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
