namespace BuildTool.Models;

/// <summary>
/// Top-level data model for a .eplugin file.
/// Represents the complete plugin descriptor parsed from JSON.
/// Maps all fields defined in REQ-007.
/// </summary>
public sealed class PluginDescriptor
{
    /// <summary>File format version (required, must be positive).</summary>
    public int FileVersion { get; init; }

    /// <summary>Internal version number for the plugin.</summary>
    public int Version { get; init; } = 1;

    /// <summary>Human-readable version string (e.g. "1.0.0").</summary>
    public string VersionName { get; init; } = string.Empty;

    /// <summary>Display name shown in plugin browsers and editors.</summary>
    public string FriendlyName { get; init; } = string.Empty;

    /// <summary>Short description of the plugin's purpose.</summary>
    public string Description { get; init; } = string.Empty;

    /// <summary>Category for organizational grouping (e.g. "Gameplay", "Rendering").</summary>
    public string Category { get; init; } = string.Empty;

    /// <summary>Author or organization name.</summary>
    public string CreatedBy { get; init; } = string.Empty;

    /// <summary>URL for the author or organization.</summary>
    public string CreatedByURL { get; init; } = string.Empty;

    /// <summary>Whether this plugin can contain content assets.</summary>
    public bool CanContainContent { get; init; }

    /// <summary>List of modules provided by this plugin.</summary>
    public List<ModuleDescriptor> Modules { get; init; } = [];

    /// <summary>List of other plugins this plugin depends on.</summary>
    public List<PluginReference> Plugins { get; init; } = [];

    /// <summary>Absolute path to the .eplugin file this descriptor was loaded from.</summary>
    public string SourceFilePath { get; set; } = string.Empty;
}
