// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Utils;

/// <summary>
/// Atomic file writer with content-change detection.
/// Only overwrites files when content differs, preserving timestamps for incremental builds.
/// Uses temp file + rename for crash safety.
/// </summary>
public static class AtomicFileWriter
{
    public enum WriteStatus
    {
        /// <summary>File was created or updated with new content.</summary>
        Written,

        /// <summary>File content was identical, write skipped.</summary>
        Unchanged,

        /// <summary>Write failed due to I/O error.</summary>
        Error
    }

    /// <summary>
    /// Write content to file atomically. Only overwrites if content differs.
    /// Creates parent directories if they don't exist.
    /// </summary>
    /// <param name="filePath">Absolute path to the target file.</param>
    /// <param name="content">UTF-8 content to write.</param>
    /// <returns>Status indicating whether the file was written, unchanged, or errored.</returns>
    public static WriteStatus WriteIfChanged(string filePath, string content)
    {
        ArgumentNullException.ThrowIfNull(filePath);
        ArgumentNullException.ThrowIfNull(content);

        string tempPath = filePath + ".tmp";

        try
        {
            // Check if existing file has identical content
            if (File.Exists(filePath))
            {
                string existing = File.ReadAllText(filePath, System.Text.Encoding.UTF8);
                if (string.Equals(existing, content, StringComparison.Ordinal))
                    return WriteStatus.Unchanged;
            }

            // Ensure parent directory exists
            string? directory = Path.GetDirectoryName(filePath);
            if (!string.IsNullOrEmpty(directory))
                Directory.CreateDirectory(directory);

            // Write to temp file first
            File.WriteAllText(tempPath, content, System.Text.Encoding.UTF8);

            // Atomic rename (overwrite if exists)
            File.Move(tempPath, filePath, overwrite: true);

            return WriteStatus.Written;
        }
        catch (IOException)
        {
            // Clean up temp file on failure
            try { if (File.Exists(tempPath)) File.Delete(tempPath); }
            catch { /* Best effort cleanup */ }

            return WriteStatus.Error;
        }
    }
}
