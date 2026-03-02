// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Models;

/// <summary>
/// Lightweight metadata for a project template variant discovered
/// under <c>Engine/Templates/Project/{Name}/</c>.
/// </summary>
public sealed class ProjectTemplateInfo
{
    /// <summary>Directory name used as the template identifier (e.g. "BlankDX12").</summary>
    public required string Name { get; init; }

    /// <summary>Human-readable display name (e.g. "Blank (DirectX 12)").</summary>
    public string DisplayName { get; set; } = string.Empty;

    /// <summary>Optional description of the template.</summary>
    public string Description { get; set; } = string.Empty;

    /// <summary>Absolute path to the template directory on disk.</summary>
    public required string DirectoryPath { get; init; }

    /// <summary>Optional tags for categorization (e.g. "rendering", "console").</summary>
    public List<string> Tags { get; set; } = [];
}
