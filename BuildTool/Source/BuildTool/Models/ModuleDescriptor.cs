namespace BuildTool.Models;

/// <summary>
/// Describes a module entry within a .eproject file.
/// Maps to the "Modules" array in the project JSON.
/// </summary>
public sealed class ModuleDescriptor
{
    /// <summary>Module name (must match the directory and .Build.cs name).</summary>
    public required string Name { get; init; }

    /// <summary>Host type determining where the module can be loaded.</summary>
    public EHostType Type { get; init; } = EHostType.Runtime;

    /// <summary>Loading phase controlling initialization order.</summary>
    public ELoadingPhase LoadingPhase { get; init; } = ELoadingPhase.Default;

    /// <summary>Additional module dependencies beyond those declared in .Build.cs.</summary>
    public List<string> AdditionalDependencies { get; init; } = [];
}
