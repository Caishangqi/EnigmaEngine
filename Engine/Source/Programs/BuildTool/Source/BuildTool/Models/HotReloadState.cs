// HotReloadState.cs -- Tracks hot-reload suffix counter and snapshot DLL mappings.
// Persisted to {ProjectRoot}/Intermediate/HotReload/HotReload.state as JSON.
// Matches UE's HotReload.state pattern.

using System.Text.Json;
using System.Text.Json.Serialization;

namespace BuildTool.Models;

/// <summary>
/// Tracks hot-reload build state: snapshot suffix counter and
/// original-to-snapshot DLL filename mappings.
/// </summary>
public class HotReloadState
{
    /// <summary>Next snapshot suffix to use (starts at 1, incremented each hot-reload build).</summary>
    [JsonPropertyName("NextSuffix")]
    public int NextSuffix { get; set; } = 1;

    /// <summary>Maps original DLL filename to its current snapshot DLL filename.</summary>
    [JsonPropertyName("OriginalToVersioned")]
    public Dictionary<string, string> OriginalToVersioned { get; set; } = new(StringComparer.OrdinalIgnoreCase);

    /// <summary>Load state from disk. Returns default state if file missing or corrupt.</summary>
    public static HotReloadState Load(string path)
    {
        try
        {
            if (File.Exists(path))
            {
                string json = File.ReadAllText(path);
                return JsonSerializer.Deserialize<HotReloadState>(json) ?? new HotReloadState();
            }
        }
        catch
        {
            // Corrupt state file -- start fresh.
        }
        return new HotReloadState();
    }

    /// <summary>Save state to disk atomically (write to temp, then rename).</summary>
    public static void Save(HotReloadState state, string path)
    {
        string? dir = Path.GetDirectoryName(path);
        if (dir is not null)
            Directory.CreateDirectory(dir);

        var options = new JsonSerializerOptions { WriteIndented = true };
        string json = JsonSerializer.Serialize(state, options);

        // Atomic write: temp file then rename.
        string tempPath = path + ".tmp";
        File.WriteAllText(tempPath, json);
        File.Move(tempPath, path, overwrite: true);
    }

    /// <summary>Get the state file path for a project.</summary>
    public static string GetStateFilePath(string projectRoot)
    {
        return Path.Combine(projectRoot, "Intermediate", "HotReload", "HotReload.state");
    }
}
