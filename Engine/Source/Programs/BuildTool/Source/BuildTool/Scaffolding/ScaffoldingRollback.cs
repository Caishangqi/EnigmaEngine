// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

/// <summary>
/// Transaction-like mechanism for atomic scaffolding operations.
/// Tracks created files, directories, and modified files. If not committed,
/// automatically rolls back all changes on disposal.
/// </summary>
public sealed class ScaffoldingRollback : IDisposable
{
    private readonly List<string> _createdFiles = [];
    private readonly List<string> _createdDirectories = [];
    private readonly Dictionary<string, byte[]> _modifiedFiles = [];
    private bool _committed;
    private bool _disposed;

    /// <summary>Track a newly created file for potential rollback (deletion).</summary>
    public void TrackFile(string filePath)
    {
        ArgumentNullException.ThrowIfNull(filePath);
        _createdFiles.Add(filePath);
    }

    /// <summary>Track a newly created directory for potential rollback (deletion).</summary>
    public void TrackDirectory(string directoryPath)
    {
        ArgumentNullException.ThrowIfNull(directoryPath);
        _createdDirectories.Add(directoryPath);
    }

    /// <summary>
    /// Track a file that will be modified. Reads and stores the original content
    /// so it can be restored on rollback.
    /// </summary>
    public void TrackModifiedFile(string filePath)
    {
        ArgumentNullException.ThrowIfNull(filePath);
        if (_modifiedFiles.ContainsKey(filePath))
            return;

        if (File.Exists(filePath))
        {
            _modifiedFiles[filePath] = File.ReadAllBytes(filePath);
        }
    }

    /// <summary>
    /// Mark the operation as committed. Prevents rollback on disposal.
    /// </summary>
    public void Commit()
    {
        _committed = true;
    }

    /// <summary>
    /// Roll back all tracked changes: delete created files, delete created directories
    /// (deepest first), and restore modified files from stored originals.
    /// Best-effort - IO exceptions are swallowed.
    /// </summary>
    public void Rollback()
    {
        // 1. Delete created files
        foreach (var file in _createdFiles)
        {
            try
            {
                if (File.Exists(file))
                    File.Delete(file);
            }
            catch (IOException)
            {
                // Best-effort cleanup
            }
        }

        // 2. Delete created directories in reverse order (deepest first)
        var sorted = _createdDirectories
            .OrderByDescending(d => d.Length)
            .ToList();

        foreach (var dir in sorted)
        {
            try
            {
                if (Directory.Exists(dir) && !Directory.EnumerateFileSystemEntries(dir).Any())
                    Directory.Delete(dir);
            }
            catch (IOException)
            {
                // Best-effort cleanup
            }
        }

        // 3. Restore modified files from stored originals
        foreach (var (filePath, originalContent) in _modifiedFiles)
        {
            try
            {
                File.WriteAllBytes(filePath, originalContent);
            }
            catch (IOException)
            {
                // Best-effort cleanup
            }
        }
    }

    /// <summary>
    /// If not committed, automatically rolls back all tracked changes.
    /// </summary>
    public void Dispose()
    {
        if (_disposed)
            return;

        _disposed = true;

        if (!_committed)
            Rollback();
    }
}
