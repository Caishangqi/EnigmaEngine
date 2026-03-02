namespace BuildTool.Models;

/// <summary>
/// Build configuration types matching engine-side EBuildConfiguration.
/// </summary>
public enum BuildConfiguration
{
    /// <summary>Full debug, no optimization.</summary>
    Debug,

    /// <summary>Engine optimized, game debug.</summary>
    DebugGame,

    /// <summary>Development build with moderate optimization.</summary>
    Development,

    /// <summary>Release build with full optimization.</summary>
    Shipping,

    /// <summary>Test build for automated testing.</summary>
    Test
}
