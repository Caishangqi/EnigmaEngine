using System.Text.Json;
using System.Text.Json.Serialization;
using BuildTool.Models;

namespace BuildTool.Parsers;

/// <summary>
/// Parses .eproject JSON files into <see cref="ProjectDescriptor"/> models.
/// Uses System.Text.Json with case-insensitive matching for Unreal-compatible PascalCase JSON.
/// </summary>
public static class ProjectParser
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        AllowTrailingCommas = true,
        Converters =
        {
            new JsonStringEnumConverter(JsonNamingPolicy.CamelCase),
        },
    };

    /// <summary>
    /// Parse a .eproject file at the given path.
    /// </summary>
    /// <param name="filePath">Absolute or relative path to the .eproject file.</param>
    /// <returns>A fully populated <see cref="ProjectDescriptor"/>.</returns>
    /// <exception cref="FileNotFoundException">The file does not exist.</exception>
    /// <exception cref="ProjectParseException">The file contains invalid JSON or missing required fields.</exception>
    public static ProjectDescriptor Parse(string filePath)
    {
        var fullPath = Path.GetFullPath(filePath);

        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException($"Project file not found: {fullPath}", fullPath);
        }

        string json;
        try
        {
            json = File.ReadAllText(fullPath);
        }
        catch (IOException ex)
        {
            throw new ProjectParseException(fullPath, $"Failed to read file: {ex.Message}", ex);
        }

        ProjectDescriptor? descriptor;
        try
        {
            descriptor = JsonSerializer.Deserialize<ProjectDescriptor>(json, JsonOptions);
        }
        catch (JsonException ex)
        {
            throw new ProjectParseException(fullPath, FormatJsonError(ex), ex);
        }

        if (descriptor is null)
        {
            throw new ProjectParseException(fullPath, "Deserialization returned null (empty or invalid JSON).");
        }

        Validate(descriptor, fullPath);

        descriptor.SourceFilePath = fullPath;
        return descriptor;
    }

    /// <summary>
    /// Validate required fields and semantic constraints.
    /// </summary>
    private static void Validate(ProjectDescriptor descriptor, string filePath)
    {
        if (descriptor.FileVersion <= 0)
        {
            throw new ProjectParseException(filePath,
                $"Invalid or missing 'FileVersion' (got {descriptor.FileVersion}). Must be a positive integer.");
        }

        for (int i = 0; i < descriptor.Modules.Count; i++)
        {
            var module = descriptor.Modules[i];
            if (string.IsNullOrWhiteSpace(module.Name))
            {
                throw new ProjectParseException(filePath,
                    $"Module at index {i} has an empty or missing 'Name'.");
            }
        }

        for (int i = 0; i < descriptor.Plugins.Count; i++)
        {
            var plugin = descriptor.Plugins[i];
            if (string.IsNullOrWhiteSpace(plugin.Name))
            {
                throw new ProjectParseException(filePath,
                    $"Plugin at index {i} has an empty or missing 'Name'.");
            }
        }
    }

    /// <summary>
    /// Format a JsonException into a user-friendly message with line/position info.
    /// </summary>
    private static string FormatJsonError(JsonException ex)
    {
        var message = $"JSON parse error: {ex.Message}";
        if (ex.LineNumber.HasValue)
        {
            message += $" (line {ex.LineNumber.Value + 1}";
            if (ex.BytePositionInLine.HasValue)
            {
                message += $", position {ex.BytePositionInLine.Value}";
            }
            message += ")";
        }
        return message;
    }
}
