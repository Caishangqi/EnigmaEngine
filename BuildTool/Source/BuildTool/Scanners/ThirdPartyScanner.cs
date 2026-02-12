using BuildTool.Models;
using BuildTool.Parsers;

namespace BuildTool.Scanners;

/// <summary>
/// Scans Engine/Source/ThirdParty/ for module Build.cs files.
/// ThirdParty modules are typically header-only and generate CMake INTERFACE targets.
/// A module is considered header-only if it has no .cpp source files in its directory tree.
/// </summary>
public static class ThirdPartyScanner
{
    /// <summary>
    /// Scan the given ThirdParty root directory for module Build.cs files.
    /// Each subdirectory containing a {Name}.Build.cs file is treated as a module.
    /// </summary>
    /// <param name="thirdPartyRoot">
    /// Absolute path to the ThirdParty directory (e.g. Engine/Source/ThirdParty).
    /// </param>
    /// <returns>
    /// Dictionary of discovered modules keyed by module name.
    /// Returns empty dictionary if the directory does not exist.
    /// </returns>
    public static Dictionary<string, ModuleRules> Scan(string thirdPartyRoot)
    {
        var result = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);

        var fullPath = Path.GetFullPath(thirdPartyRoot);
        if (!Directory.Exists(fullPath))
        {
            return result;
        }

        // Each immediate subdirectory may contain a .Build.cs file
        foreach (var moduleDir in Directory.GetDirectories(fullPath))
        {
            var buildCsFiles = Directory.GetFiles(moduleDir, "*.Build.cs", SearchOption.TopDirectoryOnly);
            if (buildCsFiles.Length == 0)
            {
                continue;
            }

            // Use the first .Build.cs found (one per module directory)
            var buildCsPath = buildCsFiles[0];

            try
            {
                var rules = ModuleParser.Parse(buildCsPath);
                rules.ModuleDirectory = Path.GetFullPath(moduleDir);

                // Determine if header-only: no .cpp files anywhere in the module tree
                rules.IsHeaderOnly = !HasSourceFiles(moduleDir);

                result[rules.ModuleName] = rules;
            }
            catch (Exception ex)
            {
                // Log warning but don't fail the entire scan for one bad module
                Console.Error.WriteLine(
                    $"Warning: Failed to parse ThirdParty module at '{buildCsPath}': {ex.Message}");
            }
        }

        return result;
    }

    /// <summary>
    /// Check whether a directory tree contains any compilable C++ source files (.cpp/.c/.cc/.cxx).
    /// </summary>
    private static bool HasSourceFiles(string directory)
    {
        string[] sourceExtensions = ["*.cpp", "*.c", "*.cc", "*.cxx"];

        foreach (var ext in sourceExtensions)
        {
            if (Directory.GetFiles(directory, ext, SearchOption.AllDirectories).Length > 0)
            {
                return true;
            }
        }

        return false;
    }
}
