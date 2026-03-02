using BuildTool.Models;

namespace BuildTool.Commands;

/// <summary>
/// Interface for CLI commands.
/// Each command encapsulates a single build tool operation.
/// </summary>
public interface ICommand
{
    /// <summary>Command name as used on the CLI (e.g. "build", "clean").</summary>
    string Name { get; }

    /// <summary>Short description shown in help text.</summary>
    string Description { get; }

    /// <summary>Execute the command with the given options.</summary>
    BuildResult Execute(BuildOptions options);
}
