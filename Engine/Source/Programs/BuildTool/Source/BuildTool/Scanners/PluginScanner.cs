using BuildTool.Models;
using BuildTool.Parsers;

namespace BuildTool.Scanners;

/// <summary>
/// Scans a Plugins/ directory for .eplugin files, filters by enabled status
/// from the project descriptor, and returns all plugin modules as regular ModuleRules.
///
/// Directory layout expected (Unreal-style):
///   Plugins/{PluginName}/{PluginName}.eplugin
///   Plugins/{PluginName}/Source/{ModuleName}/{ModuleName}.Build.cs
/// </summary>
public static class PluginScanner
{
    /// <summary>
    /// Result of a plugin scan, containing discovered modules and metadata.
    /// </summary>
    public sealed class ScanResult
    {
        /// <summary>All modules from enabled plugins, keyed by module name.</summary>
        public Dictionary<string, ModuleRules> Modules { get; init; } = new(StringComparer.Ordinal);

        /// <summary>Parsed descriptors for enabled plugins, keyed by plugin name.</summary>
        public Dictionary<string, PluginDescriptor> EnabledPlugins { get; init; } = new(StringComparer.Ordinal);

        /// <summary>Names of plugins that were discovered but disabled.</summary>
        public List<string> DisabledPlugins { get; init; } = [];
    }

    /// <summary>
    /// Scan the given Plugins root directory for .eplugin files.
    /// Only plugins that are enabled in <paramref name="projectPlugins"/> are loaded.
    /// A plugin not listed in <paramref name="projectPlugins"/> is treated as disabled.
    /// </summary>
    /// <param name="pluginsRoot">Absolute path to the Plugins directory.</param>
    /// <param name="projectPlugins">
    /// Plugin references from the .eproject file. Used to determine enabled/disabled status.
    /// </param>
    /// <returns>Scan result containing modules from enabled plugins.</returns>
    public static ScanResult Scan(string pluginsRoot, IReadOnlyList<PluginReference> projectPlugins)
    {
        var result = new ScanResult();

        var fullPath = Path.GetFullPath(pluginsRoot);
        if (!Directory.Exists(fullPath))
        {
            return result;
        }

        // Build a lookup of enabled plugin names from the project descriptor
        var enabledSet = BuildEnabledSet(projectPlugins);

        foreach (var pluginDir in Directory.GetDirectories(fullPath))
        {
            var pluginName = Path.GetFileName(pluginDir);

            // Look for {PluginName}.eplugin in the plugin root directory
            var epluginPath = Path.Combine(pluginDir, $"{pluginName}.eplugin");
            if (!File.Exists(epluginPath))
            {
                // Also try scanning for any .eplugin file
                var epluginFiles = Directory.GetFiles(pluginDir, "*.eplugin", SearchOption.TopDirectoryOnly);
                if (epluginFiles.Length == 0)
                    continue;
                epluginPath = epluginFiles[0];
            }

            // Check enabled status from project descriptor
            if (!enabledSet.Contains(pluginName))
            {
                result.DisabledPlugins.Add(pluginName);
                continue;
            }

            try
            {
                var descriptor = PluginParser.Parse(epluginPath);
                result.EnabledPlugins[pluginName] = descriptor;

                // Parse each module declared in the plugin
                ParsePluginModules(descriptor, pluginDir, result.Modules);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(
                    $"Warning: Failed to parse plugin '{pluginName}' at '{epluginPath}': {ex.Message}");
            }
        }

        return result;
    }

    /// <summary>
    /// Build a HashSet of plugin names that are explicitly enabled in the project.
    /// Only plugins with <see cref="PluginReference.Enabled"/> == true are included.
    /// </summary>
    private static HashSet<string> BuildEnabledSet(IReadOnlyList<PluginReference> projectPlugins)
    {
        var set = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var pluginRef in projectPlugins)
        {
            if (pluginRef.Enabled)
            {
                set.Add(pluginRef.Name);
            }
        }
        return set;
    }

    /// <summary>
    /// Parse Build.cs files for each module declared in the plugin descriptor.
    /// Module source is expected at: {pluginDir}/Source/{ModuleName}/{ModuleName}.Build.cs
    /// </summary>
    private static void ParsePluginModules(
        PluginDescriptor descriptor,
        string pluginDir,
        Dictionary<string, ModuleRules> modules)
    {
        foreach (var moduleDesc in descriptor.Modules)
        {
            var moduleName = moduleDesc.Name;
            var moduleSourceDir = Path.Combine(pluginDir, "Source", moduleName);
            var buildCsPath = Path.Combine(moduleSourceDir, $"{moduleName}.Build.cs");

            if (!File.Exists(buildCsPath))
            {
                Console.Error.WriteLine(
                    $"Warning: Module '{moduleName}' declared in plugin " +
                    $"'{descriptor.FriendlyName}' but Build.cs not found at '{buildCsPath}'.");
                continue;
            }

            try
            {
                var rules = ModuleParser.Parse(buildCsPath);
                rules.ModuleDirectory = Path.GetFullPath(moduleSourceDir);

                // Detect header-only (same logic as ThirdPartyScanner)
                rules.IsHeaderOnly = !HasSourceFiles(moduleSourceDir);

                modules[rules.ModuleName] = rules;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(
                    $"Warning: Failed to parse module '{moduleName}' Build.cs: {ex.Message}");
            }
        }
    }

    /// <summary>
    /// Check whether a directory tree contains any compilable C++ source files.
    /// </summary>
    private static bool HasSourceFiles(string directory)
    {
        string[] sourceExtensions = ["*.cpp", "*.c", "*.cc", "*.cxx"];
        foreach (var ext in sourceExtensions)
        {
            if (Directory.GetFiles(directory, ext, SearchOption.AllDirectories).Length > 0)
                return true;
        }
        return false;
    }
}
