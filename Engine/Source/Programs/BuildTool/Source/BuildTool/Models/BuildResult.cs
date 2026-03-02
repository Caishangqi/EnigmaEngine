namespace BuildTool.Models;

/// <summary>
/// Result of a build operation.
/// </summary>
public sealed class BuildResult
{
    /// <summary>Whether the operation completed successfully.</summary>
    public required bool Success { get; init; }

    /// <summary>Human-readable status message.</summary>
    public string Message { get; init; } = string.Empty;

    /// <summary>Error details if the operation failed.</summary>
    public string? ErrorDetail { get; init; }

    public static BuildResult Ok(string message = "Success") =>
        new() { Success = true, Message = message };

    public static BuildResult Fail(string message, string? detail = null) =>
        new() { Success = false, Message = message, ErrorDetail = detail };
}
