// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Commands;

using BuildTool.Build;
using BuildTool.Generators;
using BuildTool.Models;

/// <summary>
/// Generates an engine-only solution/workspace under Engine/Intermediate/ProjectFiles.
/// </summary>
public sealed class GenerateEngineProjectFilesCommand : ICommand
{
    public string Name => "generate-engine-project-files";

    public string Description => "Generate engine-only solution and Rider automation test run configs.";

    public BuildResult Execute(BuildOptions options)
    {
        try
        {
            var scan = EngineScanner.Scan(options.ProjectPath);
            string engineRoot = scan.EngineRoot;
            string engineIntermediateDir = Path.Combine(engineRoot, "Intermediate", "ProjectFiles");
            string buildToolCsproj = Path.GetFullPath(
                Path.Combine(engineRoot, "Source", "Programs", "BuildTool", "Source", "BuildTool", "BuildTool.csproj"));

            Console.WriteLine("[GenerateEngineProjectFiles] Generating module API headers...");
            var apiResult = new ModuleApiHeaderGenerator().Generate(scan.AllModules, engineRoot);
            if (!apiResult.Success)
            {
                return BuildResult.Fail("API header generation failed.", apiResult.Error);
            }
            Console.WriteLine($"  Generated: {apiResult.GeneratedCount}, Unchanged: {apiResult.SkippedCount}");

            Console.WriteLine("[GenerateEngineProjectFiles] Generating engine .vcxproj files...");
            var vcxprojGen = new VcxprojGenerator();
            int vcxprojGenerated = 0;

            var compilableModules = scan.AllModules
                .Where(module => !module.Value.IsHeaderOnly)
                .ToDictionary(module => module.Key, module => module.Value, StringComparer.Ordinal);

            var pluginModuleToPlugin = new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (var (pluginName, descriptor) in scan.PluginScanResult.EnabledPlugins)
            {
                foreach (var module in descriptor.Modules)
                {
                    pluginModuleToPlugin[module.Name] = pluginName;
                }
            }

            foreach (var (moduleName, rules) in compilableModules)
            {
                var includePaths = GenerateProjectFilesCommand.ComputeIncludePaths(
                    moduleName,
                    rules,
                    scan.AllModules,
                    scan.ResolveResult,
                    engineRoot);
                var preprocessorDefs = GenerateProjectFilesCommand.ComputePreprocessorDefs(moduleName);
                var sourceFiles = GenerateProjectFilesCommand.CollectSourceFiles(rules.ModuleDirectory);
                string? buildCsPath = Directory.GetFiles(rules.ModuleDirectory, "*.Build.cs").FirstOrDefault();
                string outputDir = ResolveProjectFileOutputDirectory(
                    engineRoot,
                    engineIntermediateDir,
                    moduleName,
                    pluginModuleToPlugin);

                var result = vcxprojGen.Generate(new VcxprojGenerator.ModuleProjectInput
                {
                    ModuleName = moduleName,
                    Rules = rules,
                    AllIncludePaths = includePaths,
                    PreprocessorDefinitions = preprocessorDefs,
                    SourceFiles = sourceFiles,
                    ModuleSourceRoot = rules.ModuleDirectory,
                    OutputDirectory = outputDir,
                    BuildToolCsprojPath = buildToolCsproj,
                    ProjectFilePath = engineRoot,
                    IsExecutable = moduleName.Equals("Launch", StringComparison.Ordinal),
                    ProjectName = "EnigmaEngine",
                    BuildCsPath = buildCsPath,
                });

                if (!result.Success)
                {
                    Console.WriteLine($"  Warning: Failed to generate {moduleName}.vcxproj: {result.Error}");
                }
                else
                {
                    vcxprojGenerated++;
                }
            }

            var headerOnlyModules = scan.ThirdPartyModules
                .Where(module => module.Value.IsHeaderOnly)
                .ToDictionary(module => module.Key, module => module.Value, StringComparer.Ordinal);

            int headerOnlyGenerated = 0;
            foreach (var (moduleName, rules) in headerOnlyModules)
            {
                var headerFiles = GenerateProjectFilesCommand.CollectHeaderFiles(rules.ModuleDirectory);
                string? buildCsPath = Directory.GetFiles(rules.ModuleDirectory, "*.Build.cs").FirstOrDefault();
                var result = vcxprojGen.GenerateHeaderOnly(new VcxprojGenerator.HeaderOnlyProjectInput
                {
                    ModuleName = moduleName,
                    ModuleDirectory = rules.ModuleDirectory,
                    OutputDirectory = engineIntermediateDir,
                    HeaderFiles = headerFiles,
                    BuildCsPath = buildCsPath,
                });

                if (!result.Success)
                {
                    Console.WriteLine($"  Warning: Failed to generate {moduleName}.vcxproj: {result.Error}");
                }
                else
                {
                    headerOnlyGenerated++;
                }
            }

            Console.WriteLine($"  Generated {vcxprojGenerated} .vcxproj files");
            if (headerOnlyGenerated > 0)
            {
                Console.WriteLine($"  Generated {headerOnlyGenerated} header-only .vcxproj files");
            }

            Console.WriteLine("[GenerateEngineProjectFiles] Generating engine .sln...");
            var solutionResult = new SolutionGenerator().GenerateEngineOnly(new SolutionGenerator.GenerateEngineOnlyInput
            {
                EngineRootPath = engineRoot,
                EngineModules = scan.EngineModules,
                PluginModules = scan.PluginScanResult.Modules,
                ThirdPartyModules = headerOnlyModules,
                ResolveResult = scan.ResolveResult,
                BuildToolCsprojPath = File.Exists(buildToolCsproj) ? buildToolCsproj : null,
                PluginScanResult = scan.PluginScanResult,
            });

            if (!solutionResult.Success)
            {
                return BuildResult.Fail("Engine solution generation failed.", solutionResult.Error);
            }
            Console.WriteLine($"  Solution: {solutionResult.OutputPath}");

            var riderResult = new RiderRunConfigurationGenerator()
                .GenerateAutomationTestConfigs(new RiderRunConfigurationGenerator.GenerateInput
                {
                    OutputDirectory = engineIntermediateDir,
                    BuildToolProjectPath = GenerateProjectFilesCommand.ToProjectDirPath(engineIntermediateDir, buildToolCsproj),
                    WorkingDirectory = "$PROJECT_DIR$/../..",
                    RootArgument = "$PROJECT_DIR$/../..",
                    EngineMode = true,
                    ReportDirectory = "Intermediate/AutomationTest/Reports",
                });
            if (!riderResult.Success)
            {
                return BuildResult.Fail("Rider run configuration generation failed.", riderResult.Error);
            }
            Console.WriteLine($"  RiderRunConfigs: {riderResult.GeneratedCount}");

            return BuildResult.Ok($"Engine project files generated: {solutionResult.OutputPath}");
        }
        catch (Exception ex)
        {
            return BuildResult.Fail("Engine project file generation failed.", ex.Message);
        }
    }

    private static string ResolveProjectFileOutputDirectory(
        string engineRoot,
        string engineIntermediateDir,
        string moduleName,
        IReadOnlyDictionary<string, string> pluginModuleToPlugin)
    {
        if (pluginModuleToPlugin.TryGetValue(moduleName, out var pluginName))
        {
            return Path.Combine(engineRoot, "Plugins", pluginName, "Intermediate", "ProjectFiles");
        }

        return engineIntermediateDir;
    }
}
