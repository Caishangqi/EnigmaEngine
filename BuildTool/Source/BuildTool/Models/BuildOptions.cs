namespace BuildTool.Models;

/// <summary>
/// Options passed to build commands.
/// </summary>
public sealed class BuildOptions
{
    /// <summary>Path to the .eproject file or project directory.</summary>
    public required string ProjectPath { get; init; }

    /// <summary>Build configuration (Debug, Development, Shipping, etc.).</summary>
    public BuildConfiguration Configuration { get; init; } = BuildConfiguration.Development;

    /// <summary>Target platform identifier.</summary>
    public string Platform { get; init; } = "Win64";

    /// <summary>Optional output directory override for the package command.</summary>
    public string? OutputDirectory { get; init; }

    /// <summary>Additional command-specific arguments (e.g. --name, --type for scaffolding commands).</summary>
    public IReadOnlyDictionary<string, string> ExtraArguments { get; init; } = new Dictionary<string, string>();
}
