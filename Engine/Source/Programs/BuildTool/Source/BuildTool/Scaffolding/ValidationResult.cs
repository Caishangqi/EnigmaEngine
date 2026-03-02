// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

/// <summary>
/// Result of a name validation operation.
/// </summary>
public sealed class ValidationResult
{
    /// <summary>Whether the validation passed.</summary>
    public required bool Success { get; init; }

    /// <summary>Error message if validation failed; empty if successful.</summary>
    public string Message { get; init; } = string.Empty;

    /// <summary>Create a successful validation result.</summary>
    public static ValidationResult Ok() =>
        new() { Success = true };

    /// <summary>Create a failed validation result with an error message.</summary>
    /// <param name="message">Description of the validation failure.</param>
    public static ValidationResult Fail(string message) =>
        new() { Success = false, Message = message };
}
