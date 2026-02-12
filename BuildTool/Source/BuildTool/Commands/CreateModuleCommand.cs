// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scaffolding;

/// <summary>
/// Creates a new C++ module in the project: generates source files from templates,
/// registers the module in .eproject, and adds it to the build target.
/// </summary>
public sealed class CreateModuleCommand : ICommand
{
    public string Name => "create-module";
    public string Description => "Create a new C++ module in the project.";

    public BuildResult Execute(BuildOptions options)
    {
        // 1. Parse arguments
        if (!options.ExtraArguments.TryGetValue("name", out var moduleName) || string.IsNullOrWhiteSpace(moduleName))
            return BuildResult.Fail("--name is required. Usage: create-module --project <path> --name <ModuleName>");

        var moduleType = options.ExtraArguments.GetValueOrDefault("type", "Runtime");

        try
        {
            // 2. Scan project
            Console.WriteLine("[CreateModule] Scanning project...");
            var scan = ProjectScanner.Scan(options.ProjectPath);

            // 3. Validate name
            Console.WriteLine($"[CreateModule] Validating name '{moduleName}'...");
            var context = ValidationContext.Create(
                scan.EngineModules.Keys,
                scan.GameModules.Keys);
            var validation = NameValidator.Validate(moduleName, context);
            if (!validation.Success)
                return BuildResult.Fail(validation.Message);

            // 4. Check directory conflict
            var moduleDir = Path.Combine(scan.ProjectRoot, "Source", moduleName);
            if (Directory.Exists(moduleDir))
                return BuildResult.Fail($"Directory already exists: {moduleDir}");

            // 5. Build replacements (longest keys first handled by TemplateEngine)
            var replacements = new Dictionary<string, string>
            {
                ["MODULE_NAME_UPPER"] = moduleName.ToUpperInvariant(),
                ["MODULE_NAME_API"] = moduleName.ToUpperInvariant() + "_API",
                ["MODULE_NAME"] = moduleName,
            };

            // 6. Scaffold with rollback protection
            var templateDir = Path.Combine(scan.EngineRoot, "Templates", "Module");
            using var rollback = new ScaffoldingRollback();

            Console.WriteLine("[CreateModule] Processing templates...");
            var engine = new TemplateEngine();
            var processResult = engine.Process(new TemplateContext
            {
                TemplateDir = templateDir,
                OutputDir = moduleDir,
                Replacements = replacements,
            });

            if (!processResult.Success)
                return BuildResult.Fail($"Template processing failed: {processResult.Error}");

            // Track created artifacts for rollback
            foreach (var file in processResult.CreatedFiles)
                rollback.TrackFile(file);
            foreach (var dir in processResult.CreatedDirectories)
                rollback.TrackDirectory(dir);
            rollback.TrackDirectory(moduleDir);

            // 7. Update .eproject
            Console.WriteLine("[CreateModule] Updating .eproject...");
            rollback.TrackModifiedFile(scan.EprojectPath);
            var eprojectResult = EprojectModifier.AddModule(scan.EprojectPath, moduleName, moduleType);
            if (!eprojectResult.Success)
                return BuildResult.Fail(eprojectResult.Message);

            // 8. Update .Target.cs
            Console.WriteLine("[CreateModule] Updating .Target.cs...");
            var targetPath = scan.GameTarget.SourceFilePath;
            rollback.TrackModifiedFile(targetPath);
            var targetResult = TargetFileModifier.AddModule(targetPath, moduleName);
            if (!targetResult.Success)
                return BuildResult.Fail(targetResult.Message);

            // 9. Commit — prevents rollback on dispose
            rollback.Commit();
            Console.WriteLine("[CreateModule] Changes committed.");

            // 10. Regenerate project files (warn on failure, don't rollback)
            Console.WriteLine("[CreateModule] Regenerating project files...");
            var genResult = new GenerateProjectFilesCommand().Execute(options);
            if (!genResult.Success)
                Console.WriteLine($"[CreateModule] Warning: Project file regeneration failed: {genResult.Message}");

            // 11. Summary
            Console.WriteLine();
            Console.WriteLine($"Module '{moduleName}' created successfully:");
            Console.WriteLine($"  Directory: {moduleDir}");
            Console.WriteLine($"  Files:     {processResult.CreatedFiles.Count}");
            Console.WriteLine($"  Type:      {moduleType}");

            return BuildResult.Ok($"Module '{moduleName}' created successfully.");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Module creation failed.", ex.Message);
        }
    }
}
