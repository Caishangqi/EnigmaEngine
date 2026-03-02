namespace BuildTool.Parsers;

/// <summary>
/// Exception thrown when a module build file (.Build.cs) cannot be parsed or validated.
/// </summary>
public sealed class ModuleParseException : Exception
{
    /// <summary>Absolute path to the file that caused the error.</summary>
    public string FilePath { get; }

    public ModuleParseException(string filePath, string message, Exception? inner = null)
        : base($"[{filePath}] {message}", inner)
    {
        FilePath = filePath;
    }
}
