namespace BuildTool.Parsers;

/// <summary>
/// Exception thrown when a project file (.eproject) cannot be parsed or validated.
/// Always includes the file path for diagnostic context.
/// </summary>
public sealed class ProjectParseException : Exception
{
    /// <summary>Absolute path to the file that caused the error.</summary>
    public string FilePath { get; }

    public ProjectParseException(string filePath, string message, Exception? inner = null)
        : base($"[{filePath}] {message}", inner)
    {
        FilePath = filePath;
    }
}
