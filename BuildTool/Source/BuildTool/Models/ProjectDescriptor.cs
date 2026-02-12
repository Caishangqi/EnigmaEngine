namespace BuildTool.Models;

/// <summary>
/// Top-level data model for a .eproject file.
/// Represents the complete project descriptor parsed from JSON.
/// </summary>
public sealed class ProjectDescriptor
{
    /// <summary>File format version.</summary>
    public int FileVersion { get; init; }

    /// <summary>Associated engine version or path.</summary>
    public string EngineAssociation { get; init; } = string.Empty;

    /// <summary>List of game modules defined in this project.</summary>
    public List<ModuleDescriptor> Modules { get; init; } = [];

    /// <summary>List of plugin dependencies for this project.</summary>
    public List<PluginReference> Plugins { get; init; } = [];

    /// <summary>Absolute path to the .eproject file this descriptor was loaded from.</summary>
    public string SourceFilePath { get; set; } = string.Empty;

    /// <summary>
    /// Optional staging directory for packaged builds.
    /// When set in .eproject JSON, used as the base output path for the package command
    /// (lower priority than the CLI --output flag).
    /// </summary>
    public string? StagingDirectory { get; init; }
}
