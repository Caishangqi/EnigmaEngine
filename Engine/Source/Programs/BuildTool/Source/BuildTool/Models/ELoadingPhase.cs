namespace BuildTool.Models;

/// <summary>
/// Loading phase for module initialization ordering.
/// Modules are loaded in ascending phase order during engine startup.
/// </summary>
public enum ELoadingPhase
{
    /// <summary>Loaded as early as possible (e.g. Core).</summary>
    EarliestPossible,

    /// <summary>Loaded after config system initialization (e.g. Json).</summary>
    PostConfigInit,

    /// <summary>Loaded before the loading screen (e.g. Engine, Renderer).</summary>
    PreLoadingScreen,

    /// <summary>Default loading phase for game modules.</summary>
    Default,

    /// <summary>Loaded after engine initialization (e.g. plugins).</summary>
    PostEngineInit,

    /// <summary>Not automatically loaded; must be loaded explicitly.</summary>
    None,
}
