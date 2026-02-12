namespace BuildTool.Parsers;

/// <summary>
/// Exception thrown when a plugin file (.eplugin) cannot be parsed or validated.
/// Always includes the file path for diagnostic context.
/// </summary>
public sealed class PluginParseException : Exception
{
    /// <summary>Absolute path to the file that caused the error.</summary>
    public string FilePath { get; }

    public PluginParseException(string filePath, string message, Exception? inner = null)
        : base($"[{filePath}] {message}", inner)
    {
        FilePath = filePath;
    }
}
