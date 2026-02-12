// Copyright EnigmaEngine. All Rights Reserved.

namespace BuildTool.Scaffolding;

using BuildTool.Analysis;
using BuildTool.Scanners;

/// <summary>
/// Result of a dependency safety check for module/plugin removal.
/// </summary>
public sealed class DependencyCheckResult
{
    /// <summary>List of modules that depend on the target module/plugin.</summary>
    public IReadOnlyList<string> Dependents { get; init; } = [];

    /// <summary>Whether it is safe to remove (no external dependents).</summary>
    public bool IsSafeToRemove => Dependents.Count == 0;

    /// <summary>Create a safe result (no dependents found).</summary>
    public static DependencyCheckResult Safe() => new();

    /// <summary>Create a blocked result with the list of dependent modules.</summary>
    public static DependencyCheckResult Blocked(IReadOnlyList<string> dependents) =>
        new() { Dependents = dependents };
}

/// <summary>
/// Checks reverse dependencies to determine if a module or plugin can be safely removed.
/// </summary>
public static class DependencyChecker
{
    /// <summary>
    /// Find all modules that depend on the specified module (reverse dependency lookup).
    /// </summary>
    /// <param name="moduleName">The module to check dependents for.</param>
    /// <param name="resolveResult">The dependency resolution result containing the adjacency list.</param>
    /// <returns>Safe if no dependents, Blocked with dependent list otherwise.</returns>
    public static DependencyCheckResult FindDependents(
        string moduleName,
        DependencyResolver.ResolveResult resolveResult)
    {
        var dependents = new List<string>();

        foreach (var (module, dependencies) in resolveResult.AdjacencyList)
        {
            // Skip self
            if (string.Equals(module, moduleName, StringComparison.OrdinalIgnoreCase))
                continue;

            if (dependencies.Contains(moduleName, StringComparer.OrdinalIgnoreCase))
                dependents.Add(module);
        }

        return dependents.Count == 0
            ? DependencyCheckResult.Safe()
            : DependencyCheckResult.Blocked(dependents);
    }

    /// <summary>
    /// Find all external modules that depend on any module provided by the specified plugin.
    /// </summary>
    /// <param name="pluginName">The plugin to check dependents for.</param>
    /// <param name="pluginScan">The plugin scan result containing plugin descriptors.</param>
    /// <param name="resolveResult">The dependency resolution result containing the adjacency list.</param>
    /// <returns>Safe if no external dependents, Blocked with dependent list otherwise.</returns>
    public static DependencyCheckResult FindPluginDependents(
        string pluginName,
        PluginScanner.ScanResult pluginScan,
        DependencyResolver.ResolveResult resolveResult)
    {
        // Get the plugin's own module names
        if (!pluginScan.EnabledPlugins.TryGetValue(pluginName, out var pluginDescriptor))
            return DependencyCheckResult.Safe();

        var pluginModuleNames = new HashSet<string>(
            pluginDescriptor.Modules.Select(m => m.Name),
            StringComparer.OrdinalIgnoreCase);

        if (pluginModuleNames.Count == 0)
            return DependencyCheckResult.Safe();

        // Find all external modules that depend on any of the plugin's modules
        var dependents = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        foreach (var (module, dependencies) in resolveResult.AdjacencyList)
        {
            // Skip the plugin's own modules
            if (pluginModuleNames.Contains(module))
                continue;

            foreach (var dep in dependencies)
            {
                if (pluginModuleNames.Contains(dep))
                {
                    dependents.Add(module);
                    break;
                }
            }
        }

        return dependents.Count == 0
            ? DependencyCheckResult.Safe()
            : DependencyCheckResult.Blocked(dependents.ToList());
    }
}
