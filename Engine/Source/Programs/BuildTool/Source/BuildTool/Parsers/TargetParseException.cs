namespace BuildTool.Parsers;

/// <summary>
/// Exception thrown when a target file (.Target.cs) cannot be parsed or validated.
/// </summary>
public sealed class TargetParseException : Exception
{
    /// <summary>Absolute path to the file that caused the error.</summary>
    public string FilePath { get; }

    public TargetParseException(string filePath, string message, Exception? inner = null)
        : base($"[{filePath}] {message}", inner)
    {
        FilePath = filePath;
    }
}
