// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

using BuildTool.Utils;

/// <summary>
/// Context for template processing, specifying source templates, output location, and placeholder replacements.
/// </summary>
public sealed class TemplateContext
{
    /// <summary>Absolute path to the template directory containing source files.</summary>
    public required string TemplateDir { get; init; }

    /// <summary>Absolute path to the output directory where processed files will be written.</summary>
    public required string OutputDir { get; init; }

    /// <summary>
    /// Placeholder-to-value replacements. Keys are placeholder strings (e.g. MODULE_NAME),
    /// values are the replacement text. Replacements are applied longest-key-first.
    /// </summary>
    public required IReadOnlyDictionary<string, string> Replacements { get; init; }
}

/// <summary>
/// Result of a template processing operation.
/// </summary>
public sealed class ProcessResult
{
    /// <summary>Whether the processing completed successfully.</summary>
    public required bool Success { get; init; }

    /// <summary>Absolute paths of all files created or updated.</summary>
    public IReadOnlyList<string> CreatedFiles { get; init; } = [];

    /// <summary>Absolute paths of all directories created.</summary>
    public IReadOnlyList<string> CreatedDirectories { get; init; } = [];

    /// <summary>Error message if processing failed; null if successful.</summary>
    public string? Error { get; init; }

    /// <summary>Create a successful result.</summary>
    public static ProcessResult Ok(IReadOnlyList<string> createdFiles, IReadOnlyList<string> createdDirectories) =>
        new() { Success = true, CreatedFiles = createdFiles, CreatedDirectories = createdDirectories };

    /// <summary>Create a failed result with an error message.</summary>
    public static ProcessResult Fail(string error) =>
        new() { Success = false, Error = error };
}

/// <summary>
/// Processes template directories by replacing placeholders in file paths and content,
/// then writing the results via <see cref="AtomicFileWriter"/>.
/// </summary>
public sealed class TemplateEngine
{
    /// <summary>File extension marking template files; removed from output paths.</summary>
    private const string TemplateSuffix = ".template";

    /// <summary>
    /// Process all template files in <paramref name="context"/>.TemplateDir,
    /// replacing placeholders and writing results to <paramref name="context"/>.OutputDir.
    /// </summary>
    public ProcessResult Process(TemplateContext context)
    {
        ArgumentNullException.ThrowIfNull(context);

        if (!Directory.Exists(context.TemplateDir))
        {
            return ProcessResult.Fail($"Template directory does not exist: {context.TemplateDir}");
        }

        // Sort replacements by key length descending to prevent partial matches
        var sortedReplacements = context.Replacements
            .OrderByDescending(kv => kv.Key.Length)
            .ToList();

        var createdFiles = new List<string>();
        var createdDirectories = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        try
        {
            var templateFiles = Directory.EnumerateFiles(context.TemplateDir, "*", SearchOption.AllDirectories);

            foreach (var templateFile in templateFiles)
            {
                // Compute relative path from template dir
                var relativePath = Path.GetRelativePath(context.TemplateDir, templateFile);

                // Replace placeholders in path
                var outputRelativePath = ApplyReplacements(relativePath, sortedReplacements);

                // Remove .template suffix if present
                if (outputRelativePath.EndsWith(TemplateSuffix, StringComparison.OrdinalIgnoreCase))
                {
                    outputRelativePath = outputRelativePath[..^TemplateSuffix.Length];
                }

                var outputPath = Path.Combine(context.OutputDir, outputRelativePath);

                // Track created directories
                var outputDir = Path.GetDirectoryName(outputPath);
                if (!string.IsNullOrEmpty(outputDir) && !Directory.Exists(outputDir))
                {
                    createdDirectories.Add(outputDir);
                }

                // Read template content preserving original line endings
                var content = File.ReadAllText(templateFile);

                // Replace placeholders in content
                var processedContent = ApplyReplacements(content, sortedReplacements);

                // Write via AtomicFileWriter
                var status = AtomicFileWriter.WriteIfChanged(outputPath, processedContent);
                if (status == AtomicFileWriter.WriteStatus.Error)
                {
                    return ProcessResult.Fail($"Failed to write file: {outputPath}");
                }

                createdFiles.Add(outputPath);
            }

            return ProcessResult.Ok(createdFiles, createdDirectories.ToList());
        }
        catch (IOException ex)
        {
            return ProcessResult.Fail($"Template processing failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Apply placeholder replacements to a string, using pre-sorted (longest-first) replacement pairs.
    /// </summary>
    private static string ApplyReplacements(string input, List<KeyValuePair<string, string>> sortedReplacements)
    {
        var result = input;
        foreach (var (key, value) in sortedReplacements)
        {
            result = result.Replace(key, value);
        }
        return result;
    }
}
