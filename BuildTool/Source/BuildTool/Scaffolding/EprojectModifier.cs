// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

using System.Text.Json;
using System.Text.Json.Nodes;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Utils;

/// <summary>
/// Modifies .eproject files by adding/removing module and plugin entries.
/// Uses <see cref="JsonNode"/> to preserve original file structure and unknown fields.
/// </summary>
public static class EprojectModifier
{
    private static readonly JsonSerializerOptions WriteOptions = new()
    {
        WriteIndented = true,
    };

    /// <summary>
    /// Add a module entry to the .eproject file's Modules array.
    /// </summary>
    public static BuildResult AddModule(
        string eprojectPath,
        string moduleName,
        string moduleType = "Runtime",
        string loadingPhase = "Default")
    {
        try
        {
            // Validate file structure via ProjectParser
            ProjectParser.Parse(eprojectPath);

            var root = LoadJsonNode(eprojectPath);
            if (root is null)
                return BuildResult.Fail("Failed to parse .eproject as JSON.");

            var modules = root["Modules"]?.AsArray();
            if (modules is null)
            {
                modules = new JsonArray();
                root["Modules"] = modules;
            }

            // Check for duplicate
            if (FindEntryByName(modules, moduleName) is not null)
                return BuildResult.Fail($"Module '{moduleName}' already exists in the project.");

            modules.Add(new JsonObject
            {
                ["Name"] = moduleName,
                ["Type"] = moduleType,
                ["LoadingPhase"] = loadingPhase,
            });

            return WriteBack(root, eprojectPath, $"Module '{moduleName}' added.");
        }
        catch (Exception ex) when (ex is FileNotFoundException or ProjectParseException or IOException)
        {
            return BuildResult.Fail(ex.Message);
        }
    }

    /// <summary>
    /// Add a plugin entry to the .eproject file's Plugins array.
    /// </summary>
    public static BuildResult AddPlugin(
        string eprojectPath,
        string pluginName,
        bool enabled = true)
    {
        try
        {
            ProjectParser.Parse(eprojectPath);

            var root = LoadJsonNode(eprojectPath);
            if (root is null)
                return BuildResult.Fail("Failed to parse .eproject as JSON.");

            var plugins = root["Plugins"]?.AsArray();
            if (plugins is null)
            {
                plugins = new JsonArray();
                root["Plugins"] = plugins;
            }

            if (FindEntryByName(plugins, pluginName) is not null)
                return BuildResult.Fail($"Plugin '{pluginName}' already exists in the project.");

            plugins.Add(new JsonObject
            {
                ["Name"] = pluginName,
                ["Enabled"] = enabled,
            });

            return WriteBack(root, eprojectPath, $"Plugin '{pluginName}' added.");
        }
        catch (Exception ex) when (ex is FileNotFoundException or ProjectParseException or IOException)
        {
            return BuildResult.Fail(ex.Message);
        }
    }

    /// <summary>
    /// Remove a module entry from the .eproject file's Modules array by name (case-insensitive).
    /// </summary>
    public static BuildResult RemoveModule(string eprojectPath, string moduleName)
    {
        try
        {
            ProjectParser.Parse(eprojectPath);

            var root = LoadJsonNode(eprojectPath);
            if (root is null)
                return BuildResult.Fail("Failed to parse .eproject as JSON.");

            var modules = root["Modules"]?.AsArray();
            if (modules is null)
                return BuildResult.Fail($"Module '{moduleName}' not found in the project.");

            var entry = FindEntryByName(modules, moduleName);
            if (entry is null)
                return BuildResult.Fail($"Module '{moduleName}' not found in the project.");

            modules.Remove(entry);
            return WriteBack(root, eprojectPath, $"Module '{moduleName}' removed.");
        }
        catch (Exception ex) when (ex is FileNotFoundException or ProjectParseException or IOException)
        {
            return BuildResult.Fail(ex.Message);
        }
    }

    /// <summary>
    /// Remove a plugin entry from the .eproject file's Plugins array by name (case-insensitive).
    /// </summary>
    public static BuildResult RemovePlugin(string eprojectPath, string pluginName)
    {
        try
        {
            ProjectParser.Parse(eprojectPath);

            var root = LoadJsonNode(eprojectPath);
            if (root is null)
                return BuildResult.Fail("Failed to parse .eproject as JSON.");

            var plugins = root["Plugins"]?.AsArray();
            if (plugins is null)
                return BuildResult.Fail($"Plugin '{pluginName}' not found in the project.");

            var entry = FindEntryByName(plugins, pluginName);
            if (entry is null)
                return BuildResult.Fail($"Plugin '{pluginName}' not found in the project.");

            plugins.Remove(entry);
            return WriteBack(root, eprojectPath, $"Plugin '{pluginName}' removed.");
        }
        catch (Exception ex) when (ex is FileNotFoundException or ProjectParseException or IOException)
        {
            return BuildResult.Fail(ex.Message);
        }
    }

    private static JsonObject? LoadJsonNode(string filePath)
    {
        var json = File.ReadAllText(filePath);
        return JsonNode.Parse(json, documentOptions: new JsonDocumentOptions
        {
            CommentHandling = JsonCommentHandling.Skip,
            AllowTrailingCommas = true,
        })?.AsObject();
    }

    private static JsonNode? FindEntryByName(JsonArray array, string name)
    {
        foreach (var item in array)
        {
            var entryName = item?["Name"]?.GetValue<string>();
            if (string.Equals(entryName, name, StringComparison.OrdinalIgnoreCase))
                return item;
        }
        return null;
    }

    private static BuildResult WriteBack(JsonObject root, string filePath, string successMessage)
    {
        var output = root.ToJsonString(WriteOptions);
        var status = AtomicFileWriter.WriteIfChanged(filePath, output);
        return status == AtomicFileWriter.WriteStatus.Error
            ? BuildResult.Fail($"Failed to write {filePath}")
            : BuildResult.Ok(successMessage);
    }
}
