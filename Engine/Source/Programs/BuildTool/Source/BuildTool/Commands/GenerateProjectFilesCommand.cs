// Copyright EnigmaEngine. All Rights Reserved.

using BuildTool.Analysis;
using BuildTool.Build;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Scanners;
using BuildTool.Utils;

namespace BuildTool.Commands;

/// <summary>
/// Generates Visual Studio solution and project files (.sln, .vcxproj, .vcxproj.filters)
/// and module API export headers ({Module}API.h).
/// </summary>
public sealed class GenerateProjectFilesCommand : ICommand
{
    public string Name => "generate-project-files";
    public string Description => "Generate Visual Studio solution and project files.";

    public BuildResult Execute(BuildOptions options)
    {
        try
        {
            // 1-8. Scan project: resolve paths, parse targets, scan modules, resolve dependencies
            var scan = ProjectScanner.Scan(options.ProjectPath);

            string projectRoot = scan.ProjectRoot;
            string engineRoot = scan.EngineRoot;
            string projectName = scan.ProjectName;
            string eprojectPath = scan.EprojectPath;
            var allModules = scan.AllModules;
            var resolveResult = scan.ResolveResult;
            var engineModules = scan.EngineModules;
            var gameModules = scan.GameModules;
            var thirdPartyModules = scan.ThirdPartyModules;
            var pluginScanResult = scan.PluginScanResult;
            var gameTarget = scan.GameTarget;
            var engineTarget = scan.EngineTarget;

            // 9. Generate module API headers (to Intermediate/Generated/)
            Console.WriteLine("[GenerateProjectFiles] Generating module API headers...");
            var apiHeaderGen = new ModuleApiHeaderGenerator();
            var apiResult = apiHeaderGen.Generate(allModules, projectRoot);
            if (!apiResult.Success)
                return BuildResult.Fail("API header generation failed.", apiResult.Error);
            Console.WriteLine($"  Generated: {apiResult.GeneratedCount}, Unchanged: {apiResult.SkippedCount}");

            // 10. Generate .vcxproj files for each module
            Console.WriteLine("[GenerateProjectFiles] Generating .vcxproj files...");
            string engineIntermediateDir = Path.Combine(engineRoot, "Intermediate", "ProjectFiles");
            string gameIntermediateDir = Path.Combine(projectRoot, "Intermediate", "ProjectFiles");
            string buildToolCsproj = Path.GetFullPath(
                Path.Combine(engineRoot, "Source", "Programs", "BuildTool", "Source", "BuildTool", "BuildTool.csproj"));

            int vcxprojGenerated = 0;
            var vcxprojGen = new VcxprojGenerator();

            // Determine which modules are executable targets (from TargetRules.ExtraModuleNames)
            var executableModules = new HashSet<string>(StringComparer.Ordinal);
            if (gameTarget is { Type: TargetType.Game })
            {
                foreach (var name in gameTarget.ExtraModuleNames)
                    executableModules.Add(name);
            }
            if (engineTarget is { Type: TargetType.Game })
            {
                foreach (var name in engineTarget.ExtraModuleNames)
                    executableModules.Add(name);
            }

            // Exclude header-only (third-party) modules from vcxproj generation
            var compilableModules = allModules
                .Where(m => !m.Value.IsHeaderOnly)
                .ToDictionary(m => m.Key, m => m.Value);

            // Collect .Target.cs files and assign to modules
            var targetCsMap = CollectTargetCsFiles(engineRoot, projectRoot, compilableModules);

            // Build set of engine-side module names for vcxproj directory routing
            // Includes engine runtime modules and compilable third-party modules
            var engineModuleNameSet = new HashSet<string>(engineModules.Keys, StringComparer.Ordinal);
            foreach (var name in thirdPartyModules.Keys)
                engineModuleNameSet.Add(name);

            // Build moduleName → pluginName mapping for plugin vcxproj routing
            var pluginModuleToPlugin = new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (var (pluginName, descriptor) in pluginScanResult.EnabledPlugins)
            {
                foreach (var moduleDesc in descriptor.Modules)
                    pluginModuleToPlugin[moduleDesc.Name] = pluginName;
            }

            foreach (var (moduleName, rules) in compilableModules)
            {
                var includePaths = ComputeIncludePaths(moduleName, rules, allModules, resolveResult, projectRoot);
                var preprocessorDefs = ComputePreprocessorDefs(moduleName);
                var sourceFiles = CollectSourceFiles(rules.ModuleDirectory);
                string? buildCsPath = Directory.GetFiles(rules.ModuleDirectory, "*.Build.cs").FirstOrDefault();

                // Engine modules and third-party → Engine/Intermediate/ProjectFiles/
                // Plugin modules → {ProjectRoot}/Plugins/{PluginName}/Intermediate/ProjectFiles/
                // Game modules → {ProjectRoot}/Intermediate/ProjectFiles/
                string outputDir;
                if (engineModuleNameSet.Contains(moduleName))
                    outputDir = engineIntermediateDir;
                else if (pluginModuleToPlugin.TryGetValue(moduleName, out var pluginName))
                    outputDir = Path.Combine(projectRoot, "Plugins", pluginName, "Intermediate", "ProjectFiles");
                else
                    outputDir = gameIntermediateDir;

                var vcxResult = vcxprojGen.Generate(new VcxprojGenerator.ModuleProjectInput
                {
                    ModuleName = moduleName,
                    Rules = rules,
                    AllIncludePaths = includePaths,
                    PreprocessorDefinitions = preprocessorDefs,
                    SourceFiles = sourceFiles,
                    ModuleSourceRoot = rules.ModuleDirectory,
                    OutputDirectory = outputDir,
                    BuildToolCsprojPath = buildToolCsproj,
                    ProjectFilePath = Path.GetFullPath(eprojectPath),
                    IsExecutable = executableModules.Contains(moduleName),
                    ProjectName = projectName,
                    BuildCsPath = buildCsPath,
                    TargetCsPaths = targetCsMap.TryGetValue(moduleName, out var tcs) ? tcs : [],
                });

                if (!vcxResult.Success)
                    Console.WriteLine($"  Warning: Failed to generate {moduleName}.vcxproj: {vcxResult.Error}");
                else
                    vcxprojGenerated++;
            }
            Console.WriteLine($"  Generated {vcxprojGenerated} .vcxproj files");

            // Generate header-only .vcxproj files (ThirdParty Utility projects)
            var headerOnlyModules = thirdPartyModules
                .Where(m => m.Value.IsHeaderOnly)
                .ToDictionary(m => m.Key, m => m.Value);

            int headerOnlyGenerated = 0;
            foreach (var (moduleName, rules) in headerOnlyModules)
            {
                var headerFiles = CollectHeaderFiles(rules.ModuleDirectory);
                string? buildCsPath = Directory.GetFiles(rules.ModuleDirectory, "*.Build.cs").FirstOrDefault();

                var vcxResult = vcxprojGen.GenerateHeaderOnly(new VcxprojGenerator.HeaderOnlyProjectInput
                {
                    ModuleName = moduleName,
                    ModuleDirectory = rules.ModuleDirectory,
                    OutputDirectory = engineIntermediateDir,
                    HeaderFiles = headerFiles,
                    BuildCsPath = buildCsPath,
                });

                if (!vcxResult.Success)
                    Console.WriteLine($"  Warning: Failed to generate {moduleName}.vcxproj: {vcxResult.Error}");
                else
                    headerOnlyGenerated++;
            }
            if (headerOnlyGenerated > 0)
                Console.WriteLine($"  Generated {headerOnlyGenerated} header-only .vcxproj files");

            // 11. Generate .sln
            Console.WriteLine("[GenerateProjectFiles] Generating .sln...");
            var slnGen = new SolutionGenerator();
            var slnResult = slnGen.Generate(new SolutionGenerator.GenerateInput
            {
                ProjectName = projectName,
                ProjectRootPath = projectRoot,
                EngineRootPath = engineRoot,
                EngineModules = engineModules,
                GameModules = gameModules,
                PluginModules = pluginScanResult.Modules,
                ThirdPartyModules = headerOnlyModules,
                ResolveResult = resolveResult,
                GameTarget = gameTarget,
                EngineTarget = engineTarget,
                BuildToolCsprojPath = File.Exists(buildToolCsproj) ? buildToolCsproj : null,
                PluginScanResult = pluginScanResult,
            });

            if (!slnResult.Success)
                return BuildResult.Fail("Solution generation failed.", slnResult.Error);

            Console.WriteLine($"  Solution: {slnResult.OutputPath}");

            // 12. Generate GenerateProjectFiles.bat at project root
            GenerateBatchScript(projectRoot, buildToolCsproj, eprojectPath);

            Console.WriteLine();
            Console.WriteLine($"Generated {slnResult.ProjectCount} projects, {vcxprojGenerated} .vcxproj files.");

            return BuildResult.Ok($"Project files generated: {slnResult.OutputPath}");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Project file generation failed.", ex.Message);
        }
    }

    /// <summary>Compute include paths for a module: own Public/ + dependencies' Public/ + Generated/.</summary>
    private static List<string> ComputeIncludePaths(string moduleName, ModuleRules rules,
        IReadOnlyDictionary<string, ModuleRules> allModules, DependencyResolver.ResolveResult resolveResult,
        string projectRoot)
    {
        var paths = new List<string>();

        // Auto-generated API headers (Intermediate/Generated/)
        string generatedDir = Path.Combine(projectRoot, "Intermediate", "Generated");
        paths.Add(generatedDir);

        // Own public and private include paths
        string publicDir = Path.Combine(rules.ModuleDirectory, "Public");
        if (Directory.Exists(publicDir)) paths.Add(publicDir);

        string privateDir = Path.Combine(rules.ModuleDirectory, "Private");
        if (Directory.Exists(privateDir)) paths.Add(privateDir);

        // Explicit include paths from .Build.cs
        paths.AddRange(rules.PublicIncludePaths);
        paths.AddRange(rules.PrivateIncludePaths);

        // Dependencies' public include paths
        if (resolveResult.AdjacencyList.TryGetValue(moduleName, out var deps))
        {
            foreach (var dep in deps)
            {
                if (allModules.TryGetValue(dep, out var depRules))
                {
                    string depPublic = Path.Combine(depRules.ModuleDirectory, "Public");
                    if (Directory.Exists(depPublic)) paths.Add(depPublic);

                    // For header-only modules, also add include/ directory
                    string depInclude = Path.Combine(depRules.ModuleDirectory, "include");
                    if (Directory.Exists(depInclude)) paths.Add(depInclude);

                    paths.AddRange(depRules.PublicIncludePaths);
                }
            }
        }

        return paths;
    }

    /// <summary>Compute preprocessor definitions for a module.</summary>
    private static List<string> ComputePreprocessorDefs(string moduleName)
    {
        return [$"{moduleName.ToUpperInvariant()}_EXPORTS"];
    }

    /// <summary>Collect all source files (.cpp, .h, .hpp, .c) from a module directory.</summary>
    private static List<string> CollectSourceFiles(string moduleDir)
    {
        var files = new List<string>();
        string[] extensions = ["*.cpp", "*.h", "*.hpp", "*.c"];

        foreach (var ext in extensions)
        {
            files.AddRange(Directory.GetFiles(moduleDir, ext, SearchOption.AllDirectories));
        }

        files.Sort(StringComparer.OrdinalIgnoreCase);
        return files;
    }

    /// <summary>Collect all header files (.h, .hpp) from a module directory.</summary>
    private static List<string> CollectHeaderFiles(string moduleDir)
    {
        var files = new List<string>();
        string[] extensions = ["*.h", "*.hpp"];

        foreach (var ext in extensions)
        {
            files.AddRange(Directory.GetFiles(moduleDir, ext, SearchOption.AllDirectories));
        }

        files.Sort(StringComparer.OrdinalIgnoreCase);
        return files;
    }

    /// <summary>
    /// Collect .Target.cs files and assign each to the appropriate module's vcxproj.
    /// Game .Target.cs → executable game module matching the target name prefix.
    /// Engine .Target.cs → Launch module (engine entry point).
    /// </summary>
    private static Dictionary<string, List<string>> CollectTargetCsFiles(
        string engineRoot, string projectRoot, IReadOnlyDictionary<string, ModuleRules> compilableModules)
    {
        var map = new Dictionary<string, List<string>>(StringComparer.Ordinal);

        void Assign(string moduleName, string path)
        {
            if (!compilableModules.ContainsKey(moduleName)) return;
            if (!map.TryGetValue(moduleName, out var list))
            {
                list = [];
                map[moduleName] = list;
            }
            list.Add(path);
        }

        // Engine .Target.cs → Launch module
        string engineSourceDir = Path.Combine(engineRoot, "Source");
        if (Directory.Exists(engineSourceDir))
        {
            foreach (var file in Directory.GetFiles(engineSourceDir, "*.Target.cs", SearchOption.TopDirectoryOnly))
                Assign("Launch", file);
        }

        // Game .Target.cs → match by target name prefix, fallback to first executable module
        string gameSourceDir = Path.Combine(projectRoot, "Source");
        if (Directory.Exists(gameSourceDir))
        {
            foreach (var file in Directory.GetFiles(gameSourceDir, "*.Target.cs", SearchOption.TopDirectoryOnly))
            {
                string targetName = Path.GetFileName(file).Replace(".Target.cs", "", StringComparison.OrdinalIgnoreCase);
                if (compilableModules.ContainsKey(targetName))
                    Assign(targetName, file);
                else
                    Assign("Launch", file); // fallback
            }
        }

        return map;
    }

    /// <summary>
    /// Generate GenerateProjectFiles.bat at the project root.
    /// Allows users to regenerate project files after deleting Intermediate/,
    /// matching Unreal Engine's standard workflow.
    /// </summary>
    private static void GenerateBatchScript(string projectRoot, string buildToolCsproj, string eprojectPath)
    {
        string batPath = Path.Combine(projectRoot, "GenerateProjectFiles.bat");
        string relativeCsproj = Path.GetRelativePath(projectRoot, buildToolCsproj).Replace('/', '\\');
        string relativeEproject = Path.GetRelativePath(projectRoot, Path.GetFullPath(eprojectPath)).Replace('/', '\\');

        string content = $"""
            @echo off
            echo ========================================
            echo  EnigmaEngine - Generate Project Files
            echo ========================================
            echo.
            dotnet run --project "%~dp0{relativeCsproj}" -- generate-project-files --project "%~dp0{relativeEproject}"
            if %ERRORLEVEL% NEQ 0 (
                echo.
                echo [ERROR] Project file generation failed.
                pause
                exit /b 1
            )
            echo.
            echo Project files generated successfully.
            pause
            """;

        // Normalize indentation: remove leading spaces from heredoc
        content = string.Join("\r\n",
            content.Split('\n').Select(line => line.TrimStart()));

        AtomicFileWriter.WriteIfChanged(batPath, content);
        Console.WriteLine($"  Script:   {batPath}");
    }
}