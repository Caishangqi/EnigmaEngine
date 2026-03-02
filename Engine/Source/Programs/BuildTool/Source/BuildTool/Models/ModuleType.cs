namespace BuildTool.Models;

/// <summary>
/// Classification of a module's intended usage context.
/// </summary>
public enum ModuleType
{
    /// <summary>Runtime module included in game builds.</summary>
    Runtime,

    /// <summary>Developer/test tool, not included in game builds.</summary>
    DeveloperTool,
}
