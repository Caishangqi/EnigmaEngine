namespace BuildTool.Models;

/// <summary>
/// Describes which ownership bucket a module-local automation test source belongs to.
/// </summary>
public enum AutomationTestSourceOwner
{
    Engine,
    Game,
    Plugin,
}

/// <summary>
/// Describes one module-local automation test source discovered under Private/Tests.
/// </summary>
public sealed class AutomationTestSource
{
    /// <summary>Module that owns this test source.</summary>
    public required string ModuleName { get; init; }

    /// <summary>Absolute path to the owning module directory.</summary>
    public required string ModuleDirectory { get; init; }

    /// <summary>Absolute path to the discovered .cpp test source file.</summary>
    public required string SourceFilePath { get; init; }

    /// <summary>Path to the source file relative to ModuleDirectory, using forward slashes.</summary>
    public required string RelativeSourcePath { get; init; }

    /// <summary>High-level owner bucket for filtering and future runner generation.</summary>
    public required AutomationTestSourceOwner Owner { get; init; }

    /// <summary>Plugin name when Owner is Plugin; otherwise null.</summary>
    public string? PluginName { get; init; }
}
