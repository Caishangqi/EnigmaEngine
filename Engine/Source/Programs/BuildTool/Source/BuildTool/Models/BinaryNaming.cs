// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Models;

/// <summary>
/// Centralized binary naming convention for EnigmaEngine.
///
/// Modular builds (Development/DebugGame/Debug/Test):
///   All binaries use the fixed engine prefix "EnigmaEngine".
///   DLLs:      EnigmaEngine-{Module}.dll / EnigmaEngine-{Module}-{Platform}-{Config}.dll
///   EXE:       EnigmaEngine.exe / EnigmaEngine-{Platform}-{Config}.exe
///   Manifests: EnigmaEngine.modules / EnigmaEngine-{Platform}-{Config}.modules
///
/// Shipping build (monolithic):
///   Uses {ProjectName} as prefix (game branding).
///   EXE:      {ProjectName}-{Platform}-Shipping.exe
///   Launcher: {ProjectName}.exe
/// </summary>
public static class BinaryNaming
{
    /// <summary>
    /// Fixed engine binary name used as prefix for all modular (non-Shipping) builds.
    /// Analogous to "UnrealEditor" in Unreal Engine.
    /// </summary>
    public const string EngineBinaryName = "EnigmaEngine";

    /// <summary>
    /// Get the effective binary name prefix for the given configuration.
    /// Shipping returns projectName (game branding), all others return <see cref="EngineBinaryName"/>.
    /// </summary>
    public static string GetBinaryPrefix(string projectName, BuildConfiguration configuration)
    {
        return configuration == BuildConfiguration.Shipping
            ? projectName
            : EngineBinaryName;
    }

    /// <summary>
    /// Get the DLL file name for a module.
    ///   Development: {Prefix}-{ModuleName}.dll
    ///   Others:      {Prefix}-{ModuleName}-{Platform}-{Config}.dll
    /// </summary>
    public static string GetDllFileName(
        string projectName, string moduleName,
        BuildConfiguration configuration, string platform)
    {
        var prefix = GetBinaryPrefix(projectName, configuration);

        if (configuration == BuildConfiguration.Development)
            return $"{prefix}-{moduleName}.dll";

        return $"{prefix}-{moduleName}-{platform}-{configuration}.dll";
    }

    /// <summary>
    /// Get the executable output name (without .exe extension).
    ///   Development: {Prefix}
    ///   Others:      {Prefix}-{Platform}-{Config}
    /// </summary>
    public static string GetExecutableOutputName(
        string projectName, BuildConfiguration configuration, string platform)
    {
        var prefix = GetBinaryPrefix(projectName, configuration);

        if (configuration == BuildConfiguration.Development)
            return prefix;

        return $"{prefix}-{platform}-{configuration}";
    }

    /// <summary>
    /// Get the manifest base file name (without extension).
    ///   Development: {Prefix}
    ///   Others:      {Prefix}-{Platform}-{Config}
    /// </summary>
    public static string GetManifestBaseName(
        string projectName, BuildConfiguration configuration, string platform)
    {
        var prefix = GetBinaryPrefix(projectName, configuration);

        if (configuration == BuildConfiguration.Development)
            return prefix;

        return $"{prefix}-{platform}-{configuration}";
    }

    /// <summary>
    /// Get the DLL output name for CMake set_target_properties OUTPUT_NAME (without .dll extension).
    ///   Development: {Prefix}-{ModuleName}
    ///   Others:      {Prefix}-{ModuleName}-{Platform}-{Config}
    /// </summary>
    public static string GetOutputName(
        string projectName, string moduleName,
        BuildConfiguration configuration, string platform)
    {
        var prefix = GetBinaryPrefix(projectName, configuration);

        if (configuration == BuildConfiguration.Development)
            return $"{prefix}-{moduleName}";

        return $"{prefix}-{moduleName}-{platform}-{configuration}";
    }

    // ── Hot-Reload Versioned DLL Naming ──

    /// <summary>
    /// Get a versioned DLL filename for hot-reload builds.
    /// Inserts <c>-{suffix:D4}</c> before <c>.dll</c>.
    ///   Development: {Prefix}-{Module}-0001.dll
    ///   Others:      {Prefix}-{Module}-{Platform}-{Config}-0001.dll
    /// </summary>
    public static string GetHotReloadDllFileName(
        string projectName, string moduleName,
        BuildConfiguration configuration, string platform, int suffix)
    {
        string baseName = GetOutputName(projectName, moduleName, configuration, platform);
        return $"{baseName}-{suffix:D4}.dll";
    }

    /// <summary>
    /// Check if a DLL filename has a hot-reload suffix (<c>-NNNN.dll</c>).
    /// </summary>
    public static bool HasHotReloadSuffix(string dllFileName)
    {
        // Match pattern: -NNNN.dll at end of filename
        string name = Path.GetFileNameWithoutExtension(dllFileName);
        if (name.Length < 5) return false;
        if (name[name.Length - 5] != '-') return false;
        for (int i = 1; i <= 4; i++)
        {
            if (!char.IsAsciiDigit(name[name.Length - i]))
                return false;
        }
        return true;
    }

    /// <summary>
    /// Strip the hot-reload suffix from a DLL filename.
    /// Returns the original filename, or the input unchanged if no suffix found.
    ///   "EnigmaEngine-MyGame-0001.dll" → "EnigmaEngine-MyGame.dll"
    /// </summary>
    public static string StripHotReloadSuffix(string dllFileName)
    {
        if (!HasHotReloadSuffix(dllFileName))
            return dllFileName;

        string ext = Path.GetExtension(dllFileName);
        string name = Path.GetFileNameWithoutExtension(dllFileName);
        // Remove last 5 chars: "-NNNN"
        string stripped = name[..^5];
        return stripped + ext;
    }

    /// <summary>
    /// Extract the numeric hot-reload suffix from a DLL filename.
    /// Returns null if no suffix found.
    /// </summary>
    public static int? ExtractHotReloadSuffix(string dllFileName)
    {
        if (!HasHotReloadSuffix(dllFileName))
            return null;

        string name = Path.GetFileNameWithoutExtension(dllFileName);
        string digits = name[^4..];
        return int.TryParse(digits, out int result) ? result : null;
    }
}
