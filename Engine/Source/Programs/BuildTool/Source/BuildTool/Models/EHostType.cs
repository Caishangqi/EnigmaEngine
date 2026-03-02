namespace BuildTool.Models;

/// <summary>
/// Module host type, determining when and where the module can be loaded.
/// </summary>
public enum EHostType
{
    /// <summary>Module loaded at runtime (game and standalone).</summary>
    Runtime,

    /// <summary>Module loaded only in the editor environment.</summary>
    Editor,

    /// <summary>Module loaded only in developer/debug builds.</summary>
    Developer,

    /// <summary>Module is a standalone program (not loaded by engine).</summary>
    Program,
}
