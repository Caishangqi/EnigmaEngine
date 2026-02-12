// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

/// <summary>
/// Contextual data for name validation, providing sets of existing names to check against.
/// </summary>
public sealed class ValidationContext
{
    /// <summary>Engine module names to check for conflicts (case-insensitive).</summary>
    public required HashSet<string> EngineModuleNames { get; init; }

    /// <summary>Existing project-level names (modules, plugins) to check for conflicts (case-insensitive).</summary>
    public required HashSet<string> ExistingNames { get; init; }

    /// <summary>
    /// Create a new <see cref="ValidationContext"/> with case-insensitive name sets.
    /// </summary>
    /// <param name="engineModuleNames">Engine module names.</param>
    /// <param name="existingNames">Existing project-level names.</param>
    public static ValidationContext Create(
        IEnumerable<string> engineModuleNames,
        IEnumerable<string> existingNames) =>
        new()
        {
            EngineModuleNames = new HashSet<string>(engineModuleNames, StringComparer.OrdinalIgnoreCase),
            ExistingNames = new HashSet<string>(existingNames, StringComparer.OrdinalIgnoreCase),
        };
}
