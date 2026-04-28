namespace BuildTool.Models;

/// <summary>
/// Data model for a .Build.cs file.
/// Defines module dependencies, include paths, and build rules.
/// </summary>
public sealed class ModuleRules
{
    /// <summary>Module name (extracted from the class declaration).</summary>
    public string ModuleName { get; set; } = string.Empty;

    /// <summary>Public include paths exposed to dependent modules.</summary>
    public List<string> PublicIncludePaths { get; init; } = [];

    /// <summary>Private include paths used only within this module.</summary>
    public List<string> PrivateIncludePaths { get; init; } = [];

    /// <summary>Public dependency module names (propagated to dependents).</summary>
    public List<string> PublicDependencyModuleNames { get; init; } = [];

    /// <summary>Private dependency module names (internal only).</summary>
    public List<string> PrivateDependencyModuleNames { get; init; } = [];

    /// <summary>Public dependency module names used only by automation test targets.</summary>
    public List<string> PublicTestDependencyModuleNames { get; init; } = [];

    /// <summary>Private dependency module names used only by automation test targets.</summary>
    public List<string> PrivateTestDependencyModuleNames { get; init; } = [];

    /// <summary>Module names loaded dynamically at runtime.</summary>
    public List<string> DynamicallyLoadedModuleNames { get; init; } = [];

    /// <summary>Absolute path to the .Build.cs file this was parsed from.</summary>
    public string SourceFilePath { get; set; } = string.Empty;

    /// <summary>
    /// Whether this module is header-only (no compiled sources).
    /// Header-only modules generate CMake INTERFACE targets instead of SHARED libraries.
    /// They only propagate include paths and dependencies to consumers.
    /// </summary>
    public bool IsHeaderOnly { get; set; }

    /// <summary>
    /// Module type classification. Runtime modules are included in game builds;
    /// DeveloperTool modules are for testing/development only.
    /// Default is Runtime.
    /// </summary>
    public ModuleType Type { get; set; } = ModuleType.Runtime;

    /// <summary>
    /// Absolute path to the module's root directory.
    /// For header-only modules, include paths are resolved relative to this directory.
    /// Derived from SourceFilePath if not explicitly set.
    /// </summary>
    public string ModuleDirectory { get; set; } = string.Empty;
}
