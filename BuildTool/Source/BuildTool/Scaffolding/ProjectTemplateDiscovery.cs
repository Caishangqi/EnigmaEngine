// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

using System.Text.Json;
using BuildTool.Models;

/// <summary>
/// Discovers project template variants by scanning subdirectories
/// under <c>Engine/Templates/Project/</c>.
/// Each subdirectory is treated as a template; an optional <c>template.json</c>
/// provides metadata (DisplayName, Description, Tags).
/// </summary>
public static class ProjectTemplateDiscovery
{
    /// <summary>Relative path from engine root to the project templates directory.</summary>
    private static readonly string TemplatesRelativePath = Path.Combine("Engine", "Templates", "Project");

    /// <summary>Name of the optional metadata file inside each template directory.</summary>
    private const string MetadataFileName = "template.json";

    /// <summary>Name of the default template directory (always listed first).</summary>
    private const string DefaultTemplateName = "Default";

    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
    };

    /// <summary>
    /// Discover all project templates under <c>Engine/Templates/Project/</c>.
    /// Returns "Default" as the first entry (if present), followed by remaining
    /// templates in alphabetical order.
    /// Returns an empty list if the templates directory does not exist.
    /// </summary>
    public static List<ProjectTemplateInfo> Discover(string engineRoot)
    {
        ArgumentException.ThrowIfNullOrEmpty(engineRoot);

        var templatesDir = Path.Combine(engineRoot, TemplatesRelativePath);
        if (!Directory.Exists(templatesDir))
        {
            return [];
        }

        var subdirectories = Directory.GetDirectories(templatesDir);
        var templates = new List<ProjectTemplateInfo>(subdirectories.Length);
        ProjectTemplateInfo? defaultTemplate = null;

        foreach (var subdir in subdirectories.OrderBy(d => Path.GetFileName(d), StringComparer.OrdinalIgnoreCase))
        {
            var info = BuildTemplateInfo(subdir);
            if (string.Equals(info.Name, DefaultTemplateName, StringComparison.OrdinalIgnoreCase))
            {
                defaultTemplate = info;
            }
            else
            {
                templates.Add(info);
            }
        }

        // Default always comes first.
        if (defaultTemplate is not null)
        {
            templates.Insert(0, defaultTemplate);
        }

        return templates;
    }

    /// <summary>
    /// Find a specific template by name (case-insensitive).
    /// Returns <c>null</c> if the template directory does not exist.
    /// </summary>
    public static ProjectTemplateInfo? Find(string engineRoot, string templateName)
    {
        ArgumentException.ThrowIfNullOrEmpty(engineRoot);
        ArgumentException.ThrowIfNullOrEmpty(templateName);

        var templatesDir = Path.Combine(engineRoot, TemplatesRelativePath);
        if (!Directory.Exists(templatesDir))
        {
            return null;
        }

        var subdirectories = Directory.GetDirectories(templatesDir);
        var match = subdirectories.FirstOrDefault(d =>
            string.Equals(Path.GetFileName(d), templateName, StringComparison.OrdinalIgnoreCase));

        return match is not null ? BuildTemplateInfo(match) : null;
    }

    /// <summary>
    /// Build a <see cref="ProjectTemplateInfo"/> from a template subdirectory,
    /// parsing <c>template.json</c> if present.
    /// </summary>
    private static ProjectTemplateInfo BuildTemplateInfo(string directoryPath)
    {
        var name = Path.GetFileName(directoryPath);
        var info = new ProjectTemplateInfo
        {
            Name = name,
            DisplayName = name,
            DirectoryPath = directoryPath,
        };

        var metadataPath = Path.Combine(directoryPath, MetadataFileName);
        if (!File.Exists(metadataPath))
        {
            return info;
        }

        try
        {
            var json = File.ReadAllText(metadataPath);
            var metadata = JsonSerializer.Deserialize<TemplateMetadata>(json, JsonOptions);
            if (metadata is null)
            {
                return info;
            }

            if (!string.IsNullOrWhiteSpace(metadata.DisplayName))
            {
                info.DisplayName = metadata.DisplayName;
            }

            if (!string.IsNullOrWhiteSpace(metadata.Description))
            {
                info.Description = metadata.Description;
            }

            if (metadata.Tags is { Count: > 0 })
            {
                info.Tags = metadata.Tags;
            }
        }
        catch (JsonException)
        {
            // Malformed template.json — fall back to directory-name defaults.
        }
        catch (IOException)
        {
            // Unreadable file — fall back to directory-name defaults.
        }

        return info;
    }

    /// <summary>
    /// Internal DTO for deserializing <c>template.json</c>.
    /// </summary>
    private sealed class TemplateMetadata
    {
        public string? DisplayName { get; set; }
        public string? Description { get; set; }
        public List<string>? Tags { get; set; }
    }
}
