// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Generators;

using System.Text;
using BuildTool.Analysis;
using BuildTool.Models;
using BuildTool.Utils;

/// <summary>
/// Generates Visual Studio .sln files with solution folder hierarchy matching Unreal Engine layout.
/// </summary>
public sealed class SolutionGenerator
{
    /// <summary>Well-known type GUIDs for .sln project entries.</summary>
    private static class TypeGuids
    {
        public const string SolutionFolder = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";
        public const string CppProject     = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
        public const string CSharpProject  = "{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}";
    }

    /// <summary>Fixed solution configurations for EnigmaEngine.</summary>
    private static readonly (string Config, string Platform)[] SolutionConfigs =
    [
        ("DebugGame Game",   "Win64"),
        ("Development Game", "Win64"),
        ("Shipping Game",    "Win64"),
        ("Package Game",     "Win64"),
    ];

    public sealed class GenerateInput
    {
        public required string ProjectName { get; init; }
        public required string ProjectRootPath { get; init; }
        public required string EngineRootPath { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> EngineModules { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> GameModules { get; init; }
        public required IReadOnlyDictionary<string, ModuleRules> PluginModules { get; init; }
        public IReadOnlyDictionary<string, ModuleRules> ThirdPartyModules { get; init; }
            = new Dictionary<string, ModuleRules>();
        public required DependencyResolver.ResolveResult ResolveResult { get; init; }
        public required TargetRules GameTarget { get; init; }
        public TargetRules? EngineTarget { get; init; }
        public string? BuildToolCsprojPath { get; init; }
        public IReadOnlyList<string> RuleFiles { get; init; } = [];
    }

    public sealed class GenerateResult
    {
        public required bool Success { get; init; }
        public string? Error { get; init; }
        public string OutputPath { get; init; } = string.Empty;
        public int ProjectCount { get; init; }
    }
    public GenerateResult Generate(GenerateInput input)
    {
        try
        {
            var sb = new StringBuilder();
            var allProjects = new List<(string typeGuid, string name, string path, string guid, string? parentGuid, List<string>? deps)>();

            // --- Solution Folders ---
            string engineFolderGuid   = FormatGuid(GuidGenerator.GenerateForFolder("Engine"));
            string engineSrcGuid      = FormatGuid(GuidGenerator.GenerateForFolder("Engine/Source"));
            string gamesFolderGuid    = FormatGuid(GuidGenerator.GenerateForFolder("Games"));
            string gameProjectGuid    = FormatGuid(GuidGenerator.GenerateForFolder($"Games/{input.ProjectName}"));
            string gameSrcGuid        = FormatGuid(GuidGenerator.GenerateForFolder($"Games/{input.ProjectName}/Source"));
            string pluginsFolderGuid  = FormatGuid(GuidGenerator.GenerateForFolder("Plugins"));
            string thirdPartyFolderGuid = FormatGuid(GuidGenerator.GenerateForFolder("Engine/ThirdParty"));
            string programsFolderGuid = FormatGuid(GuidGenerator.GenerateForFolder("Programs"));
            string rulesFolderGuid    = FormatGuid(GuidGenerator.GenerateForFolder("Rules"));

            var folders = new (string guid, string name, string? parent)[]
            {
                (engineFolderGuid,   "Engine",           null),
                (engineSrcGuid,      "Source",            engineFolderGuid),
                (thirdPartyFolderGuid, "ThirdParty",     engineFolderGuid),
                (gamesFolderGuid,    "Games",             null),
                (gameProjectGuid,    input.ProjectName,   gamesFolderGuid),
                (gameSrcGuid,        "Source",            gameProjectGuid),
                (pluginsFolderGuid,  "Plugins",           null),
                (programsFolderGuid, "Programs",          null),
                (rulesFolderGuid,    "Rules",             null),
            };

            // Nesting map: child GUID → parent GUID
            var nesting = new List<(string child, string parent)>();
            foreach (var (guid, _, parent) in folders)
            {
                if (parent is not null)
                    nesting.Add((guid, parent));
            }

            int projectCount = 0;

            // Collect all module names that will have .vcxproj entries in the solution.
            // Header-only modules (e.g. nlohmann_json) are NOT in these dictionaries,
            // so their GUIDs must not appear as project dependencies.
            var knownProjectNames = new HashSet<string>(StringComparer.Ordinal);
            foreach (var name in input.EngineModules.Keys) knownProjectNames.Add(name);
            foreach (var name in input.GameModules.Keys) knownProjectNames.Add(name);
            foreach (var name in input.PluginModules.Keys) knownProjectNames.Add(name);
            foreach (var name in input.ThirdPartyModules.Keys) knownProjectNames.Add(name);

            // --- Header ---
            sb.AppendLine();
            sb.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
            sb.AppendLine("# Visual Studio Version 17");
            sb.AppendLine("VisualStudioVersion = 17.0.31314.256");
            sb.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");

            // --- Solution Folder entries ---
            foreach (var (guid, name, _) in folders)
            {
                sb.AppendLine($"Project(\"{TypeGuids.SolutionFolder}\") = \"{name}\", \"{name}\", \"{guid}\"");
                sb.AppendLine("EndProject");
            }

            // --- Engine module projects ---
            string intermediateDir = Path.Combine(input.ProjectRootPath, "Intermediate", "ProjectFiles");
            foreach (var (moduleName, _) in input.EngineModules)
            {
                string projGuid = FormatGuid(GuidGenerator.GenerateForProject(moduleName));
                string vcxprojRelative = $"Intermediate\\ProjectFiles\\{moduleName}.vcxproj";
                var deps = GetProjectDependencies(moduleName, input.ResolveResult, knownProjectNames);

                WriteProjectEntry(sb, TypeGuids.CppProject, moduleName, vcxprojRelative, projGuid, deps);
                nesting.Add((projGuid, engineSrcGuid));
                projectCount++;
            }

            // --- Game module projects ---
            foreach (var (moduleName, _) in input.GameModules)
            {
                string projGuid = FormatGuid(GuidGenerator.GenerateForProject(moduleName));
                string vcxprojRelative = $"Intermediate\\ProjectFiles\\{moduleName}.vcxproj";
                var deps = GetProjectDependencies(moduleName, input.ResolveResult, knownProjectNames);

                WriteProjectEntry(sb, TypeGuids.CppProject, moduleName, vcxprojRelative, projGuid, deps);
                nesting.Add((projGuid, gameSrcGuid));
                projectCount++;
            }

            // --- Plugin module projects ---
            foreach (var (moduleName, _) in input.PluginModules)
            {
                string projGuid = FormatGuid(GuidGenerator.GenerateForProject(moduleName));
                string vcxprojRelative = $"Intermediate\\ProjectFiles\\{moduleName}.vcxproj";
                var deps = GetProjectDependencies(moduleName, input.ResolveResult, knownProjectNames);

                WriteProjectEntry(sb, TypeGuids.CppProject, moduleName, vcxprojRelative, projGuid, deps);
                nesting.Add((projGuid, pluginsFolderGuid));
                projectCount++;
            }

            // --- ThirdParty module projects (header-only, Utility type) ---
            foreach (var (moduleName, _) in input.ThirdPartyModules)
            {
                string projGuid = FormatGuid(GuidGenerator.GenerateForProject(moduleName));
                string vcxprojRelative = $"Intermediate\\ProjectFiles\\{moduleName}.vcxproj";

                WriteProjectEntry(sb, TypeGuids.CppProject, moduleName, vcxprojRelative, projGuid, null);
                nesting.Add((projGuid, thirdPartyFolderGuid));
                projectCount++;
            }

            // --- BuildTool C# project (Programs folder) ---
            if (input.BuildToolCsprojPath is not null)
            {
                string btGuid = FormatGuid(GuidGenerator.GenerateForProject("BuildTool"));
                string btRelative = Path.GetRelativePath(input.ProjectRootPath, input.BuildToolCsprojPath)
                    .Replace('/', '\\');
                WriteProjectEntry(sb, TypeGuids.CSharpProject, "BuildTool", btRelative, btGuid, null);
                nesting.Add((btGuid, programsFolderGuid));
                projectCount++;
            }

            // --- Rules project (solution folder with .Build.cs and .Target.cs) ---
            if (input.RuleFiles.Count > 0)
            {
                string rulesGuid = FormatGuid(GuidGenerator.GenerateForProject("Rules"));
                sb.AppendLine($"Project(\"{TypeGuids.SolutionFolder}\") = \"Rules Files\", \"Rules Files\", \"{rulesGuid}\"");
                sb.AppendLine("\tProjectSection(SolutionItems) = preProject");
                foreach (var ruleFile in input.RuleFiles)
                {
                    string relative = Path.GetRelativePath(input.ProjectRootPath, ruleFile).Replace('/', '\\');
                    sb.AppendLine($"\t\t{relative} = {relative}");
                }
                sb.AppendLine("\tEndProjectSection");
                sb.AppendLine("EndProject");
                nesting.Add((rulesGuid, rulesFolderGuid));
                projectCount++;
            }

            // --- Global section ---
            sb.AppendLine("Global");

            // SolutionConfigurationPlatforms
            sb.AppendLine("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution");
            foreach (var (config, platform) in SolutionConfigs)
                sb.AppendLine($"\t\t{config}|{platform} = {config}|{platform}");
            sb.AppendLine("\tEndGlobalSection");

            // ProjectConfigurationPlatforms
            sb.AppendLine("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution");

            // C++ projects: map to solution configs, Package config Build.0 only for executable modules
            var executableModules = new HashSet<string>(
                input.GameTarget?.ExtraModuleNames ?? [], StringComparer.Ordinal);

            foreach (var moduleName in EnumerateCppModuleNames(input))
            {
                string projGuid = FormatGuid(GuidGenerator.GenerateForProject(moduleName));
                foreach (var (config, platform) in SolutionConfigs)
                {
                    bool isPackage = config.StartsWith("Package", StringComparison.Ordinal);
                    sb.AppendLine($"\t\t{projGuid}.{config}|{platform}.ActiveCfg = {config}|{platform}");
                    if (!isPackage || executableModules.Contains(moduleName))
                        sb.AppendLine($"\t\t{projGuid}.{config}|{platform}.Build.0 = {config}|{platform}");
                }
            }

            // ThirdParty (Utility) projects: ActiveCfg only, no Build.0 (not compilable)
            foreach (var name in input.ThirdPartyModules.Keys)
            {
                string projGuid = FormatGuid(GuidGenerator.GenerateForProject(name));
                foreach (var (config, platform) in SolutionConfigs)
                {
                    sb.AppendLine($"\t\t{projGuid}.{config}|{platform}.ActiveCfg = {config}|{platform}");
                }
            }

            // Non-C++ projects (BuildTool C#, Rules folder): map all configs to Development|Any CPU
            // This matches UE's pattern where C# projects always use Development|Any CPU.
            var nonCppProjectGuids = CollectNonCppProjectGuids(input);
            foreach (var projGuid in nonCppProjectGuids)
            {
                foreach (var (config, platform) in SolutionConfigs)
                {
                    sb.AppendLine($"\t\t{projGuid}.{config}|{platform}.ActiveCfg = Development|Any CPU");
                    sb.AppendLine($"\t\t{projGuid}.{config}|{platform}.Build.0 = Development|Any CPU");
                }
            }

            sb.AppendLine("\tEndGlobalSection");

            // SolutionProperties
            sb.AppendLine("\tGlobalSection(SolutionProperties) = preSolution");
            sb.AppendLine("\t\tHideSolutionNode = FALSE");
            sb.AppendLine("\tEndGlobalSection");

            // NestedProjects
            sb.AppendLine("\tGlobalSection(NestedProjects) = preSolution");
            foreach (var (child, parent) in nesting)
                sb.AppendLine($"\t\t{child} = {parent}");
            sb.AppendLine("\tEndGlobalSection");

            sb.AppendLine("EndGlobal");

            // Write output
            string outputPath = Path.Combine(input.ProjectRootPath, $"{input.ProjectName}.sln");
            AtomicFileWriter.WriteIfChanged(outputPath, sb.ToString());

            return new GenerateResult
            {
                Success = true,
                OutputPath = outputPath,
                ProjectCount = projectCount,
            };
        }
        catch (Exception ex)
        {
            return new GenerateResult
            {
                Success = false,
                Error = ex.Message,
            };
        }
    }

    private static void WriteProjectEntry(StringBuilder sb, string typeGuid, string name,
        string relativePath, string projGuid, List<string>? deps)
    {
        sb.AppendLine($"Project(\"{typeGuid}\") = \"{name}\", \"{relativePath}\", \"{projGuid}\"");
        if (deps is { Count: > 0 })
        {
            sb.AppendLine("\tProjectSection(ProjectDependencies) = postProject");
            foreach (var depGuid in deps)
                sb.AppendLine($"\t\t{depGuid} = {depGuid}");
            sb.AppendLine("\tEndProjectSection");
        }
        sb.AppendLine("EndProject");
    }

    private static List<string>? GetProjectDependencies(string moduleName,
        DependencyResolver.ResolveResult resolveResult, HashSet<string> knownProjectNames)
    {
        if (resolveResult.AdjacencyList.TryGetValue(moduleName, out var depNames) && depNames.Count > 0)
        {
            var guids = depNames
                .Where(dep => knownProjectNames.Contains(dep))
                .Select(dep => FormatGuid(GuidGenerator.GenerateForProject(dep)))
                .ToList();
            return guids.Count > 0 ? guids : null;
        }
        return null;
    }

    /// <summary>Enumerate all C++ module names (engine, game, plugin).</summary>
    private static IEnumerable<string> EnumerateCppModuleNames(GenerateInput input)
    {
        foreach (var name in input.EngineModules.Keys) yield return name;
        foreach (var name in input.GameModules.Keys) yield return name;
        foreach (var name in input.PluginModules.Keys) yield return name;
    }

    /// <summary>Collect GUIDs for non-C++ projects (BuildTool C#, Rules folder).</summary>
    private static List<string> CollectNonCppProjectGuids(GenerateInput input)
    {
        var guids = new List<string>();

        if (input.BuildToolCsprojPath is not null)
            guids.Add(FormatGuid(GuidGenerator.GenerateForProject("BuildTool")));
        if (input.RuleFiles.Count > 0)
            guids.Add(FormatGuid(GuidGenerator.GenerateForProject("Rules")));

        return guids;
    }

    private static string FormatGuid(Guid guid)
        => $"{{{guid.ToString().ToUpperInvariant()}}}";
}
