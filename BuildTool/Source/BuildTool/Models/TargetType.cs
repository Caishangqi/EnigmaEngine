namespace BuildTool.Models;

/// <summary>
/// Build target type. Determines the output binary type.
/// Currently only Game is supported.
/// </summary>
public enum TargetType
{
    /// <summary>Standalone game executable.</summary>
    Game,

    // Future: Editor, Server, Program, etc.
}
