// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Generators;

using System.Text;
using BuildTool.Models;
using BuildTool.Utils;

/// <summary>
/// Generates .vcxproj and .vcxproj.filters files for each module.
/// Uses Makefile project type (ConfigurationType=Makefile) matching Unreal Engine.
/// NMake commands invoke BuildTool for actual compilation.
/// </summary>
public sealed class VcxprojGenerator
{
    private const string MsBuildNamespace = "http://schemas.microsoft.com/developer/msbuild/2003";

    /// <summary>Configuration name to preprocessor macro and optimization mapping.</summary>
    private static readonly (string SolutionConfig, string BuildConfig, string Macros, string Optimization)[] Configurations =
    [
        ("DebugGame Game|Win64",   "DebugGame",   "ENIGMA_BUILD_DEBUGGAME=1",          "/Od"),
        ("Development Game|Win64", "Development", "ENIGMA_BUILD_DEVELOPMENT=1",        "/O1"),
        ("Shipping Game|Win64",    "Shipping",    "ENIGMA_BUILD_SHIPPING=1;NDEBUG",    "/O2"),
        ("Package Game|Win64",     "Shipping",    "ENIGMA_BUILD_SHIPPING=1;NDEBUG",    "/O2"),
    ];

    public sealed class ModuleProjectInput
    {
        public required string ModuleName { get; init; }
        public required ModuleRules Rules { get; init; }
        public required IReadOnlyList<string> AllIncludePaths { get; init; }
        public required IReadOnlyList<string> PreprocessorDefinitions { get; init; }
        public required IReadOnlyList<string> SourceFiles { get; init; }
        public required string ModuleSourceRoot { get; init; }
        public required string OutputDirectory { get; init; }
        public required string BuildToolCsprojPath { get; init; }
        public required string ProjectFilePath { get; init; }

        /// <summary>Whether this module produces an executable (from TargetRules.ExtraModuleNames).</summary>
        public bool IsExecutable { get; init; }

        /// <summary>Project name for DLL/EXE output naming (e.g. "EnigmaArcade").</summary>
        public string ProjectName { get; init; } = string.Empty;
    }

    /// <summary>Input for generating a header-only (Utility) module .vcxproj.</summary>
    public sealed class HeaderOnlyProjectInput
    {
        public required string ModuleName { get; init; }
        public required string ModuleDirectory { get; init; }
        public required string OutputDirectory { get; init; }
        public required IReadOnlyList<string> HeaderFiles { get; init; }
        public string? BuildCsPath { get; init; }
    }

    public sealed class GenerateResult
    {
        public required bool Success { get; init; }
        public string? Error { get; init; }
        public string VcxprojPath { get; init; } = string.Empty;
        public string FiltersPath { get; init; } = string.Empty;
        public string UserFilePath { get; init; } = string.Empty;
    }

    /// <summary>Generate .vcxproj, .vcxproj.filters, and .vcxproj.user for a single module.</summary>
    public GenerateResult Generate(ModuleProjectInput input)
    {
        try
        {
            Directory.CreateDirectory(input.OutputDirectory);

            string vcxprojPath = Path.Combine(input.OutputDirectory, $"{input.ModuleName}.vcxproj");
            string filtersPath = Path.Combine(input.OutputDirectory, $"{input.ModuleName}.vcxproj.filters");
            string userFilePath = Path.Combine(input.OutputDirectory, $"{input.ModuleName}.vcxproj.user");

            string vcxprojContent = GenerateVcxproj(input);
            string filtersContent = GenerateFilters(input);
            string userContent = GenerateUserFile(input);

            AtomicFileWriter.WriteIfChanged(vcxprojPath, vcxprojContent);
            AtomicFileWriter.WriteIfChanged(filtersPath, filtersContent);
            AtomicFileWriter.WriteIfChanged(userFilePath, userContent);

            return new GenerateResult
            {
                Success = true,
                VcxprojPath = vcxprojPath,
                FiltersPath = filtersPath,
                UserFilePath = userFilePath,
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

    /// <summary>Generate .vcxproj and .vcxproj.filters for a header-only (Utility) module. No .vcxproj.user is needed.</summary>
    public GenerateResult GenerateHeaderOnly(HeaderOnlyProjectInput input)
    {
        try
        {
            Directory.CreateDirectory(input.OutputDirectory);

            string vcxprojPath = Path.Combine(input.OutputDirectory, $"{input.ModuleName}.vcxproj");
            string filtersPath = Path.Combine(input.OutputDirectory, $"{input.ModuleName}.vcxproj.filters");

            string vcxprojContent = GenerateHeaderOnlyVcxproj(input);
            string filtersContent = GenerateHeaderOnlyFilters(input);

            AtomicFileWriter.WriteIfChanged(vcxprojPath, vcxprojContent);
            AtomicFileWriter.WriteIfChanged(filtersPath, filtersContent);

            return new GenerateResult
            {
                Success = true,
                VcxprojPath = vcxprojPath,
                FiltersPath = filtersPath,
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

    /// <summary>
    /// Resolve the full path to dotnet.exe. VS/Rider MSBuild environments may not have dotnet in PATH.
    /// </summary>
    private static string ResolveDotnetPath()
    {
        // 1. Try the running process itself (we are invoked via `dotnet run`)
        string? processPath = Environment.ProcessPath;
        if (processPath is not null && Path.GetFileNameWithoutExtension(processPath)
                .Equals("dotnet", StringComparison.OrdinalIgnoreCase))
            return processPath;

        // 2. Well-known install location on Windows
        string wellKnown = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "dotnet", "dotnet.exe");
        if (File.Exists(wellKnown))
            return wellKnown;

        // 3. Fallback: bare name (hope PATH works)
        return "dotnet";
    }

    /// <summary>
    /// Detect system C++ include paths (MSVC + Windows SDK) for IntelliSense.
    /// Matches UE's DefaultSystemIncludePaths pattern in UECommon.props.
    /// </summary>
    private static string ResolveSystemIncludePaths()
    {
        var paths = new List<string>();

        // 1. MSVC include path: find latest version
        string vsBase = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
            "Microsoft Visual Studio", "2022");

        string? msvcInclude = null;
        if (Directory.Exists(vsBase))
        {
            foreach (var edition in new[] { "Community", "Professional", "Enterprise" })
            {
                string msvcRoot = Path.Combine(vsBase, edition, "VC", "Tools", "MSVC");
                if (!Directory.Exists(msvcRoot)) continue;

                // Pick the latest version directory
                var latest = Directory.GetDirectories(msvcRoot)
                    .OrderByDescending(d => Path.GetFileName(d))
                    .FirstOrDefault();
                if (latest is not null)
                {
                    msvcInclude = Path.Combine(latest, "INCLUDE");
                    if (Directory.Exists(msvcInclude))
                    {
                        paths.Add(msvcInclude);
                        break;
                    }
                }
            }
        }

        // 2. Windows SDK include paths (ucrt, shared, um, winrt)
        string sdkRoot = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Windows Kits", "10", "include");

        if (Directory.Exists(sdkRoot))
        {
            var latestSdk = Directory.GetDirectories(sdkRoot)
                .Where(d => Path.GetFileName(d).StartsWith("10."))
                .OrderByDescending(d => Path.GetFileName(d))
                .FirstOrDefault();

            if (latestSdk is not null)
            {
                foreach (var sub in new[] { "ucrt", "shared", "um", "winrt" })
                {
                    string subDir = Path.Combine(latestSdk, sub);
                    if (Directory.Exists(subDir))
                        paths.Add(subDir);
                }
            }
        }

        return string.Join(";", paths);
    }

    private static string GenerateVcxproj(ModuleProjectInput input)
    {
        var sb = new StringBuilder();
        var projectGuid = GuidGenerator.GenerateForProject(input.ModuleName);
        string macroPrefix = input.ModuleName.ToUpperInvariant();
        string dotnet = ResolveDotnetPath();
        string systemIncludePaths = ResolveSystemIncludePaths();

        // Header
        sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        sb.AppendLine($"<Project DefaultTargets=\"Build\" ToolsVersion=\"17.0\" xmlns=\"{MsBuildNamespace}\">");

        // ProjectConfigurations
        sb.AppendLine("  <ItemGroup Label=\"ProjectConfigurations\">");
        foreach (var (solutionConfig, _, _, _) in Configurations)
        {
            string config = solutionConfig.Split('|')[0];
            string platform = solutionConfig.Split('|')[1];
            sb.AppendLine($"    <ProjectConfiguration Include=\"{solutionConfig}\">");
            sb.AppendLine($"      <Configuration>{config}</Configuration>");
            sb.AppendLine($"      <Platform>{platform}</Platform>");
            sb.AppendLine("    </ProjectConfiguration>");
        }
        sb.AppendLine("  </ItemGroup>");

        // Globals
        sb.AppendLine("  <PropertyGroup Label=\"Globals\">");
        sb.AppendLine($"    <ProjectGuid>{{{projectGuid.ToString().ToUpperInvariant()}}}</ProjectGuid>");
        sb.AppendLine($"    <RootNamespace>{input.ModuleName}</RootNamespace>");
        sb.AppendLine("    <Keyword>MakeFileProj</Keyword>");
        sb.AppendLine($"    <ProjectName>{input.ModuleName}</ProjectName>");
        sb.AppendLine("  </PropertyGroup>");

        sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />");

        // Per-config Configuration properties
        foreach (var (solutionConfig, _, _, _) in Configurations)
        {
            sb.AppendLine($"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{solutionConfig}'\" Label=\"Configuration\">");
            sb.AppendLine("    <ConfigurationType>Makefile</ConfigurationType>");
            sb.AppendLine("    <PlatformToolset>v143</PlatformToolset>");
            sb.AppendLine("  </PropertyGroup>");
        }

        sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />");

        // Per-config NMake properties
        string includePaths = string.Join(";", input.AllIncludePaths);

        foreach (var (solutionConfig, buildConfig, configMacros, _) in Configurations)
        {
            string allMacros = $"{macroPrefix}_EXPORTS;{configMacros}";
            foreach (var def in input.PreprocessorDefinitions)
            {
                if (!allMacros.Contains(def))
                    allMacros += $";{def}";
            }

            // Compute NMakeOutput path (relative from Intermediate/ProjectFiles/)
            string nmakeOutput = ComputeNMakeOutput(input.ModuleName, input.ProjectName, input.IsExecutable, buildConfig);

            bool isPackage = solutionConfig.StartsWith("Package", StringComparison.Ordinal);

            sb.AppendLine($"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{solutionConfig}'\">");
            if (isPackage)
            {
                sb.AppendLine($"    <NMakeBuildCommandLine>\"{dotnet}\" run --project \"{input.BuildToolCsprojPath}\" -- package --project \"{input.ProjectFilePath}\"</NMakeBuildCommandLine>");
                sb.AppendLine($"    <NMakeReBuildCommandLine>\"{dotnet}\" run --project \"{input.BuildToolCsprojPath}\" -- package --project \"{input.ProjectFilePath}\"</NMakeReBuildCommandLine>");
                sb.AppendLine($"    <NMakeCleanCommandLine>\"{dotnet}\" run --project \"{input.BuildToolCsprojPath}\" -- clean --project \"{input.ProjectFilePath}\" -c Shipping</NMakeCleanCommandLine>");
            }
            else
            {
                sb.AppendLine($"    <NMakeBuildCommandLine>\"{dotnet}\" run --project \"{input.BuildToolCsprojPath}\" -- build --project \"{input.ProjectFilePath}\" -c {buildConfig}</NMakeBuildCommandLine>");
                sb.AppendLine($"    <NMakeReBuildCommandLine>\"{dotnet}\" run --project \"{input.BuildToolCsprojPath}\" -- rebuild --project \"{input.ProjectFilePath}\" -c {buildConfig}</NMakeReBuildCommandLine>");
                sb.AppendLine($"    <NMakeCleanCommandLine>\"{dotnet}\" run --project \"{input.BuildToolCsprojPath}\" -- clean --project \"{input.ProjectFilePath}\" -c {buildConfig}</NMakeCleanCommandLine>");
            }
            sb.AppendLine($"    <NMakeOutput>{nmakeOutput}</NMakeOutput>");
            // IntDir/OutDir required by Microsoft.CppClean.targets (FoldersToClean parameter)
            sb.AppendLine($@"    <OutDir>..\..\Binaries\Win64\</OutDir>");
            sb.AppendLine($@"    <IntDir>..\..\Intermediate\Build\{buildConfig}\</IntDir>");
            sb.AppendLine($"    <NMakePreprocessorDefinitions>{allMacros};%(NMakePreprocessorDefinitions)</NMakePreprocessorDefinitions>");
            sb.AppendLine($"    <IncludePath>{includePaths};$(IncludePath);{systemIncludePaths}</IncludePath>");
            sb.AppendLine("    <AdditionalOptions>/std:c++latest</AdditionalOptions>");
            sb.AppendLine("  </PropertyGroup>");
        }

        // Source files
        var cppFiles = input.SourceFiles.Where(f => IsCppFile(f)).ToList();
        var headerFiles = input.SourceFiles.Where(f => IsHeaderFile(f)).ToList();

        if (cppFiles.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var file in cppFiles)
            {
                string relativePath = GetRelativePath(input.OutputDirectory, file);
                sb.AppendLine($"    <ClCompile Include=\"{relativePath}\" />");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        if (headerFiles.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var file in headerFiles)
            {
                string relativePath = GetRelativePath(input.OutputDirectory, file);
                sb.AppendLine($"    <ClInclude Include=\"{relativePath}\" />");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
        // Override CppClean target: Makefile projects delegate cleaning to NMakeCleanCommandLine.
        // Without this, Microsoft.CppClean.targets fails with MSB4044 because
        // FilePatternsToDeleteOnClean / FoldersToClean are not set for Makefile projects.
        sb.AppendLine("  <Target Name=\"CppClean\" />");
        sb.AppendLine("</Project>");

        return sb.ToString();
    }

    private static string GenerateFilters(ModuleProjectInput input)
    {
        var sb = new StringBuilder();

        sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        sb.AppendLine($"<Project ToolsVersion=\"17.0\" xmlns=\"{MsBuildNamespace}\">");

        // Collect all unique filter directories
        var filters = new SortedSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var file in input.SourceFiles)
        {
            string? filterPath = GetFilterPath(input.ModuleSourceRoot, file);
            if (filterPath is null) continue;

            // Add all parent paths as well
            string current = filterPath;
            while (!string.IsNullOrEmpty(current))
            {
                filters.Add(current);
                current = Path.GetDirectoryName(current)?.Replace('/', '\\') ?? string.Empty;
            }
        }

        // Filter definitions
        if (filters.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var filter in filters)
            {
                var filterGuid = GuidGenerator.GenerateForFilter(input.ModuleName, filter);
                sb.AppendLine($"    <Filter Include=\"{filter}\">");
                sb.AppendLine($"      <UniqueIdentifier>{{{filterGuid.ToString().ToUpperInvariant()}}}</UniqueIdentifier>");
                sb.AppendLine("    </Filter>");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        // ClCompile entries with filters
        var cppFiles = input.SourceFiles.Where(f => IsCppFile(f)).ToList();
        if (cppFiles.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var file in cppFiles)
            {
                string relativePath = GetRelativePath(input.OutputDirectory, file);
                string? filterPath = GetFilterPath(input.ModuleSourceRoot, file);
                sb.AppendLine($"    <ClCompile Include=\"{relativePath}\">");
                sb.AppendLine($"      <Filter>{filterPath ?? ""}</Filter>");
                sb.AppendLine("    </ClCompile>");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        // ClInclude entries with filters
        var headerFiles = input.SourceFiles.Where(f => IsHeaderFile(f)).ToList();
        if (headerFiles.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var file in headerFiles)
            {
                string relativePath = GetRelativePath(input.OutputDirectory, file);
                string? filterPath = GetFilterPath(input.ModuleSourceRoot, file);
                sb.AppendLine($"    <ClInclude Include=\"{relativePath}\">");
                sb.AppendLine($"      <Filter>{filterPath ?? ""}</Filter>");
                sb.AppendLine("    </ClInclude>");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        sb.AppendLine("</Project>");

        return sb.ToString();
    }

    /// <summary>Get relative path from output directory to source file, using backslashes.</summary>
    private static string GetRelativePath(string fromDir, string toFile)
    {
        string relative = Path.GetRelativePath(fromDir, toFile);
        return relative.Replace('/', '\\');
    }

    /// <summary>Get filter path (directory relative to module source root), using backslashes.</summary>
    private static string? GetFilterPath(string moduleSourceRoot, string filePath)
    {
        string? dir = Path.GetDirectoryName(filePath);
        if (dir is null) return null;

        string relative = Path.GetRelativePath(moduleSourceRoot, dir);
        if (relative == ".") return null;

        return relative.Replace('/', '\\');
    }

    private static bool IsCppFile(string path)
        => path.EndsWith(".cpp", StringComparison.OrdinalIgnoreCase)
        || path.EndsWith(".c", StringComparison.OrdinalIgnoreCase);

    private static bool IsHeaderFile(string path)
        => path.EndsWith(".h", StringComparison.OrdinalIgnoreCase)
        || path.EndsWith(".hpp", StringComparison.OrdinalIgnoreCase);

    /// <summary>
    /// Compute NMakeOutput path relative from Intermediate/ProjectFiles/.
    /// Follows CMakeGenerator naming convention (REQ-015):
    ///   Executable Development: ..\..\Binaries\Win64\{ProjectName}.exe
    ///   Executable Other:       ..\..\Binaries\Win64\{ProjectName}-Win64-{Config}.exe
    ///   Library Development:    ..\..\Binaries\Win64\{ProjectName}-{ModuleName}.dll
    ///   Library Other:          ..\..\Binaries\Win64\{ProjectName}-{ModuleName}-Win64-{Config}.dll
    /// </summary>
    private static string ComputeNMakeOutput(string moduleName, string projectName, bool isExecutable, string buildConfig)
    {
        const string binDir = @"..\..\Binaries\Win64";
        bool isDevelopment = buildConfig.Equals("Development", StringComparison.OrdinalIgnoreCase);

        if (isExecutable)
        {
            string exeName = isDevelopment
                ? $"{projectName}.exe"
                : $"{projectName}-Win64-{buildConfig}.exe";
            return $@"{binDir}\{exeName}";
        }

        string dllName = isDevelopment
            ? $"{projectName}-{moduleName}.dll"
            : $"{projectName}-{moduleName}-Win64-{buildConfig}.dll";
        return $@"{binDir}\{dllName}";
    }

    private static string GenerateHeaderOnlyVcxproj(HeaderOnlyProjectInput input)
    {
        var sb = new StringBuilder();
        var projectGuid = GuidGenerator.GenerateForProject(input.ModuleName);

        sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        sb.AppendLine($"<Project DefaultTargets=\"Build\" ToolsVersion=\"17.0\" xmlns=\"{MsBuildNamespace}\">");

        // ProjectConfigurations
        sb.AppendLine("  <ItemGroup Label=\"ProjectConfigurations\">");
        foreach (var (solutionConfig, _, _, _) in Configurations)
        {
            string config = solutionConfig.Split('|')[0];
            string platform = solutionConfig.Split('|')[1];
            sb.AppendLine($"    <ProjectConfiguration Include=\"{solutionConfig}\">");
            sb.AppendLine($"      <Configuration>{config}</Configuration>");
            sb.AppendLine($"      <Platform>{platform}</Platform>");
            sb.AppendLine("    </ProjectConfiguration>");
        }
        sb.AppendLine("  </ItemGroup>");

        // Globals
        sb.AppendLine("  <PropertyGroup Label=\"Globals\">");
        sb.AppendLine($"    <ProjectGuid>{{{projectGuid.ToString().ToUpperInvariant()}}}</ProjectGuid>");
        sb.AppendLine($"    <RootNamespace>{input.ModuleName}</RootNamespace>");
        sb.AppendLine($"    <ProjectName>{input.ModuleName}</ProjectName>");
        sb.AppendLine("  </PropertyGroup>");

        sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />");

        // Per-config: ConfigurationType=Utility, no NMake commands
        foreach (var (solutionConfig, _, _, _) in Configurations)
        {
            sb.AppendLine($"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{solutionConfig}'\" Label=\"Configuration\">");
            sb.AppendLine("    <ConfigurationType>Utility</ConfigurationType>");
            sb.AppendLine("    <PlatformToolset>v143</PlatformToolset>");
            sb.AppendLine("  </PropertyGroup>");
        }

        sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />");

        // Build.cs as None item
        if (input.BuildCsPath is not null)
        {
            sb.AppendLine("  <ItemGroup>");
            string relativePath = GetRelativePath(input.OutputDirectory, input.BuildCsPath);
            sb.AppendLine($"    <None Include=\"{relativePath}\" />");
            sb.AppendLine("  </ItemGroup>");
        }

        // Header files as ClInclude
        if (input.HeaderFiles.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var file in input.HeaderFiles)
            {
                string relativePath = GetRelativePath(input.OutputDirectory, file);
                sb.AppendLine($"    <ClInclude Include=\"{relativePath}\" />");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
        sb.AppendLine("</Project>");

        return sb.ToString();
    }

    private static string GenerateHeaderOnlyFilters(HeaderOnlyProjectInput input)
    {
        var sb = new StringBuilder();

        sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        sb.AppendLine($"<Project ToolsVersion=\"17.0\" xmlns=\"{MsBuildNamespace}\">");

        // Collect all unique filter directories from header files
        var filters = new SortedSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var file in input.HeaderFiles)
        {
            string? filterPath = GetFilterPath(input.ModuleDirectory, file);
            if (filterPath is null) continue;

            string current = filterPath;
            while (!string.IsNullOrEmpty(current))
            {
                filters.Add(current);
                current = Path.GetDirectoryName(current)?.Replace('/', '\\') ?? string.Empty;
            }
        }

        // Filter definitions
        if (filters.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var filter in filters)
            {
                var filterGuid = GuidGenerator.GenerateForFilter(input.ModuleName, filter);
                sb.AppendLine($"    <Filter Include=\"{filter}\">");
                sb.AppendLine($"      <UniqueIdentifier>{{{filterGuid.ToString().ToUpperInvariant()}}}</UniqueIdentifier>");
                sb.AppendLine("    </Filter>");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        // Build.cs as None (root level, no filter)
        if (input.BuildCsPath is not null)
        {
            sb.AppendLine("  <ItemGroup>");
            string relativePath = GetRelativePath(input.OutputDirectory, input.BuildCsPath);
            sb.AppendLine($"    <None Include=\"{relativePath}\" />");
            sb.AppendLine("  </ItemGroup>");
        }

        // Header files with filters
        if (input.HeaderFiles.Count > 0)
        {
            sb.AppendLine("  <ItemGroup>");
            foreach (var file in input.HeaderFiles)
            {
                string relativePath = GetRelativePath(input.OutputDirectory, file);
                string? filterPath = GetFilterPath(input.ModuleDirectory, file);
                sb.AppendLine($"    <ClInclude Include=\"{relativePath}\">");
                sb.AppendLine($"      <Filter>{filterPath ?? ""}</Filter>");
                sb.AppendLine("    </ClInclude>");
            }
            sb.AppendLine("  </ItemGroup>");
        }

        sb.AppendLine("</Project>");

        return sb.ToString();
    }

    /// <summary>
    /// Generate .vcxproj.user file with debugger configuration.
    /// Sets DebuggerFlavor=WindowsLocalDebugger so VS can launch/debug the project.
    /// </summary>
    private static string GenerateUserFile(ModuleProjectInput input)
    {
        var sb = new StringBuilder();

        sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        sb.AppendLine($"<Project ToolsVersion=\"17.0\" xmlns=\"{MsBuildNamespace}\">");

        foreach (var (solutionConfig, _, _, _) in Configurations)
        {
            sb.AppendLine($"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{solutionConfig}'\">");
            sb.AppendLine("    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>");
            sb.AppendLine("    <LocalDebuggerWorkingDirectory>$(SolutionDir)</LocalDebuggerWorkingDirectory>");
            sb.AppendLine("  </PropertyGroup>");
        }

        sb.AppendLine("</Project>");

        return sb.ToString();
    }
}