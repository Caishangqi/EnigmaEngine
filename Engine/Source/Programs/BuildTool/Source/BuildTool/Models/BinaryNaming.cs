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
}
