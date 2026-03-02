namespace BuildTool.Models;

/// <summary>
/// Describes a plugin reference within a .eproject file.
/// Maps to the "Plugins" array in the project JSON.
/// </summary>
public sealed class PluginReference
{
    /// <summary>Plugin name (must match the .eplugin file name).</summary>
    public required string Name { get; init; }

    /// <summary>Whether this plugin is enabled for the project.</summary>
    public bool Enabled { get; init; }
}
