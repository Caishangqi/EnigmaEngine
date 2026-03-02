using BuildTool.Models;

namespace BuildTool;

/// <summary>
/// Core interface for the build tool operations.
/// Defines the contract for parsing project files and executing build actions.
/// </summary>
public interface IBuildTool
{
    /// <summary>Execute a full build for the specified project.</summary>
    BuildResult Build(BuildOptions options);

    /// <summary>Clean all build artifacts for the specified project.</summary>
    BuildResult Clean(BuildOptions options);

    /// <summary>Clean and then rebuild the specified project.</summary>
    BuildResult Rebuild(BuildOptions options);
}
