using System.Text.Json;
using System.Text.Json.Serialization;
using BuildTool.Models;

namespace BuildTool.Parsers;

/// <summary>
/// Parses .eplugin JSON files into <see cref="PluginDescriptor"/> models.
/// Uses System.Text.Json with case-insensitive matching for PascalCase JSON.
/// Validates required fields and handles optional fields with sensible defaults.
/// </summary>
public static class PluginParser
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
    /// Parse a .eplugin file at the given path.
    /// </summary>
    /// <param name="filePath">Absolute or relative path to the .eplugin file.</param>
    /// <returns>A fully populated <see cref="PluginDescriptor"/>.</returns>
    /// <exception cref="FileNotFoundException">The file does not exist.</exception>
    /// <exception cref="PluginParseException">The file contains invalid JSON or missing required fields.</exception>
    public static PluginDescriptor Parse(string filePath)
    {
        var fullPath = Path.GetFullPath(filePath);

        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException($"Plugin file not found: {fullPath}", fullPath);
        }

        string json;
        try
        {
            json = File.ReadAllText(fullPath);
        }
        catch (IOException ex)
        {
            throw new PluginParseException(fullPath, $"Failed to read file: {ex.Message}", ex);
        }

        PluginDescriptor? descriptor;
        try
        {
            descriptor = JsonSerializer.Deserialize<PluginDescriptor>(json, JsonOptions);
        }
        catch (JsonException ex)
        {
            throw new PluginParseException(fullPath, FormatJsonError(ex), ex);
        }

        if (descriptor is null)
        {
            throw new PluginParseException(fullPath,
                "Deserialization returned null (empty or invalid JSON).");
        }

        Validate(descriptor, fullPath);

        descriptor.SourceFilePath = fullPath;
        return descriptor;
    }

    /// <summary>
    /// Validate required fields and semantic constraints.
    /// </summary>
    private static void Validate(PluginDescriptor descriptor, string filePath)
    {
        // FileVersion is required and must be positive
        if (descriptor.FileVersion <= 0)
        {
            throw new PluginParseException(filePath,
                $"Invalid or missing 'FileVersion' (got {descriptor.FileVersion}). Must be a positive integer.");
        }

        // FriendlyName is required for plugin identification
        if (string.IsNullOrWhiteSpace(descriptor.FriendlyName))
        {
            throw new PluginParseException(filePath,
                "Missing or empty 'FriendlyName'. Every plugin must have a display name.");
        }

        // Validate module entries
        for (int i = 0; i < descriptor.Modules.Count; i++)
        {
            var module = descriptor.Modules[i];
            if (string.IsNullOrWhiteSpace(module.Name))
            {
                throw new PluginParseException(filePath,
                    $"Module at index {i} has an empty or missing 'Name'.");
            }
        }

        // Validate plugin dependency entries
        for (int i = 0; i < descriptor.Plugins.Count; i++)
        {
            var plugin = descriptor.Plugins[i];
            if (string.IsNullOrWhiteSpace(plugin.Name))
            {
                throw new PluginParseException(filePath,
                    $"Plugin dependency at index {i} has an empty or missing 'Name'.");
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
