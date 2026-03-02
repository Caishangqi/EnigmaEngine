namespace BuildTool.Models;

/// <summary>
/// Data model for a .Target.cs file.
/// Defines the build target type and which modules to include.
/// </summary>
public sealed class TargetRules
{
    /// <summary>Target type (Game, Editor, etc.). Currently only Game is supported.</summary>
    public TargetType Type { get; set; } = TargetType.Game;

    /// <summary>Default build settings version string (e.g. "V5").</summary>
    public string DefaultBuildSettings { get; set; } = string.Empty;

    /// <summary>Extra module names to load for this target.</summary>
    public List<string> ExtraModuleNames { get; init; } = [];

    /// <summary>Name of the target class (extracted from the class declaration).</summary>
    public string TargetName { get; set; } = string.Empty;

    /// <summary>Absolute path to the .Target.cs file this was parsed from.</summary>
    public string SourceFilePath { get; set; } = string.Empty;
}
