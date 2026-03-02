using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Phase 6 end-to-end plugin integration test.
/// Validates the full pipeline: .eproject parse -> plugin discovery ->
/// module parse -> dependency resolve -> CMake generate -> compile -> execute.
///
///   [1]  .eproject parsed with plugin reference
///   [2]  .Target.cs parsed (Game target)
///   [3]  Game module .Build.cs parsed (depends on TestGreeter)
///   [4]  PluginScanner discovers TestGreeter plugin
///   [5]  TestGreeter plugin is in EnabledPlugins
///   [6]  TestGreeter module parsed from plugin
///   [7]  Plugin module merged into module graph
///   [8]  Dependency resolution succeeds (no cycles)
///   [9]  Build order: TestGreeter before game module
///   [10] CMake generation succeeds
///   [11] CMake contains SHARED target for TestGreeter
///   [12] CMake contains executable for game module
///   [13] CMake contains TESTGREETER_EXPORTS definition
///   [14] CMake links game module to TestGreeter
///   [15] CMake configure succeeds
///   [16] CMake build succeeds
///   [17] Executable runs with exit code 0
///   [18] Plugin exported function returns expected result
/// </summary>
public static class Phase6PluginIntegrationTest
{
    private static int _passed;
    private static int _failed;

    public static void Run()
    {
        _passed = 0;
        _failed = 0;

        Console.WriteLine("=== Phase 6: Plugin Integration End-to-End ===");
        Console.WriteLine();

        var projectRoot = ResolveProjectRoot();
        Console.WriteLine($"Project root: {projectRoot}");
        Console.WriteLine();

        // Step 1: Parse .eproject
        TestParseProject(projectRoot);

        // Step 2: Parse .Target.cs
        TestParseTarget(projectRoot);

        // Step 3-7: Plugin discovery and module parsing
        var allModules = TestPluginDiscovery(projectRoot);

        // Step 8-9: Dependency resolution
        var resolveResult = TestDependencyResolution(allModules);

        // Step 10-14: CMake generation
        TestCMakeGeneration(allModules, resolveResult, projectRoot);

        // Step 15-18: Compile and execute
        TestCompileAndExecute(projectRoot);

        Console.WriteLine();
        Console.WriteLine($"=== {_passed}/{_passed + _failed} tests passed ===");

        if (_failed > 0)
            throw new Exception($"Phase6PluginIntegrationTest: {_failed} test(s) failed.");
    }

    private static void TestParseProject(string projectRoot)
    {
        Console.WriteLine("[Step 1] Parsing .eproject ...");
        var desc = ProjectParser.Parse(
            Path.Combine(projectRoot, "Phase6_PluginIntegration.eproject"));

        Check(desc.Modules.Count == 1
           && desc.Modules[0].Name == "Phase6_PluginIntegration"
           && desc.Plugins.Count == 1
           && desc.Plugins[0].Name == "TestGreeter"
           && desc.Plugins[0].Enabled,
            "[1]  .eproject parsed with plugin reference (TestGreeter enabled)");
    }

    private static void TestParseTarget(string projectRoot)
    {
        Console.WriteLine("[Step 2] Parsing .Target.cs ...");
        var target = TargetParser.Parse(
            Path.Combine(projectRoot, "Source", "Phase6_PluginIntegration.Target.cs"));

        Check(target.Type == TargetType.Game
           && target.ExtraModuleNames.Contains("Phase6_PluginIntegration"),
            "[2]  .Target.cs parsed (Game target)");
    }

    private static Dictionary<string, ModuleRules> TestPluginDiscovery(string projectRoot)
    {
        Console.WriteLine("[Steps 3-7] Plugin discovery and module parsing ...");

        // Parse game module Build.cs
        var gameBuildCs = Path.Combine(projectRoot, "Source",
            "Phase6_PluginIntegration", "Phase6_PluginIntegration.Build.cs");
        var gameRules = ModuleParser.Parse(gameBuildCs);

        Check(gameRules.PublicDependencyModuleNames.Contains("TestGreeter"),
            "[3]  Game module .Build.cs parsed (depends on TestGreeter)");

        // Parse .eproject for plugin references
        var desc = ProjectParser.Parse(
            Path.Combine(projectRoot, "Phase6_PluginIntegration.eproject"));

        // Scan plugins directory
        var pluginsDir = Path.Combine(projectRoot, "Plugins");
        var scanResult = PluginScanner.Scan(pluginsDir, desc.Plugins);

        Check(scanResult.EnabledPlugins.Count >= 1,
            "[4]  PluginScanner discovers TestGreeter plugin");

        Check(scanResult.EnabledPlugins.ContainsKey("TestGreeter"),
            "[5]  TestGreeter plugin is in EnabledPlugins");

        Check(scanResult.Modules.ContainsKey("TestGreeter"),
            "[6]  TestGreeter module parsed from plugin");

        // Merge into unified module graph
        var allModules = new Dictionary<string, ModuleRules>
        {
            [gameRules.ModuleName] = gameRules,
        };
        foreach (var (name, rules) in scanResult.Modules)
        {
            allModules[name] = rules;
        }

        Check(allModules.ContainsKey("TestGreeter")
           && allModules.ContainsKey("Phase6_PluginIntegration"),
            "[7]  Plugin module merged into module graph");

        return allModules;
    }

    private static DependencyResolver.ResolveResult TestDependencyResolution(
        Dictionary<string, ModuleRules> allModules)
    {
        Console.WriteLine("[Steps 8-9] Dependency resolution ...");

        var resolver = new DependencyResolver();
        var result = resolver.Resolve(allModules);

        Check(result.Success,
            "[8]  Dependency resolution succeeds (no cycles)");

        var order = result.BuildOrder.ToList();
        var greeterIdx = order.IndexOf("TestGreeter");
        var gameIdx = order.IndexOf("Phase6_PluginIntegration");
        Check(greeterIdx >= 0 && gameIdx >= 0 && greeterIdx < gameIdx,
            "[9]  Build order: TestGreeter before game module");

        Console.WriteLine($"     Build order: [{string.Join(", ", order)}]");
        return result;
    }

    private static void TestCMakeGeneration(
        Dictionary<string, ModuleRules> allModules,
        DependencyResolver.ResolveResult resolveResult,
        string projectRoot)
    {
        Console.WriteLine("[Steps 10-14] CMake generation ...");

        var target = TargetParser.Parse(
            Path.Combine(projectRoot, "Source", "Phase6_PluginIntegration.Target.cs"));

        var gen = new CMakeGenerator();
        var genResult = gen.Generate(
            "Phase6_PluginIntegration", allModules, resolveResult, projectRoot, target);

        Check(genResult.Success,
            "[10] CMake generation succeeds");

        Check(genResult.Content.Contains("add_library(TestGreeter"),
            "[11] CMake contains library target for TestGreeter");

        Check(genResult.Content.Contains("add_executable(Phase6_PluginIntegration"),
            "[12] CMake contains executable for game module");

        Check(genResult.Content.Contains("TESTGREETER_EXPORTS"),
            "[13] CMake contains TESTGREETER_EXPORTS definition");

        Check(genResult.Content.Contains("target_link_libraries(Phase6_PluginIntegration")
           && genResult.Content.Contains("TestGreeter"),
            "[14] CMake links game module to TestGreeter");
    }

    private static void TestCompileAndExecute(string projectRoot)
    {
        Console.WriteLine("[Steps 15-18] Compile and execute ...");

        // Generate API headers to Intermediate/Generated/ before compiling
        var apiGen = new Generators.ModuleApiHeaderGenerator();
        var apiModules = new Dictionary<string, Models.ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["TestGreeter"] = new() { ModuleName = "TestGreeter" },
        };
        apiGen.Generate(apiModules, projectRoot);

        var buildDir = Path.Combine(projectRoot, "Intermediate", "Build");

        // CMake configure
        var configureResult = RunProcess("cmake",
            $"-S \"{projectRoot}\" -B \"{buildDir}\" " +
            "-G \"Visual Studio 17 2022\" -A x64");

        Check(configureResult.ExitCode == 0,
            "[15] CMake configure succeeds");

        if (configureResult.ExitCode != 0)
        {
            Console.Error.WriteLine(configureResult.Output);
            return;
        }

        // CMake build
        var buildResult = RunProcess("cmake",
            $"--build \"{buildDir}\" --config Release");

        Check(buildResult.ExitCode == 0,
            "[16] CMake build succeeds");

        if (buildResult.ExitCode != 0)
        {
            Console.Error.WriteLine(buildResult.Output);
            return;
        }

        // Execute
        var exePath = Path.Combine(buildDir, "Bin", "Release", "Phase6_PluginIntegration.exe");
        var runResult = RunProcess(exePath, "");

        Check(runResult.ExitCode == 0,
            "[17] Executable runs with exit code 0");

        Console.WriteLine($"     Output: {runResult.Output.Trim()}");

        Check(runResult.Output.Contains("PLUGIN_CALL=OK")
           && runResult.Output.Contains("Hello, EnigmaEngine! (from TestGreeter plugin)"),
            "[18] Plugin exported function returns expected result");
    }

    // -- Helpers --

    private static string ResolveProjectRoot()
    {
        var baseDir = AppContext.BaseDirectory;
        // Navigate from build output to Tests/Phase6_PluginIntegration
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", "..", "..",
            "Tests", "Phase6_PluginIntegration"));

        if (Directory.Exists(candidate))
            return candidate;

        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            var testDir = Path.Combine(dir.FullName, "Tests", "Phase6_PluginIntegration");
            if (Directory.Exists(testDir))
                return testDir;
            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("Phase6_PluginIntegration directory not found.");
    }

    private static (int ExitCode, string Output) RunProcess(string fileName, string arguments)
    {
        var psi = new System.Diagnostics.ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var proc = System.Diagnostics.Process.Start(psi)!;
        var stdout = proc.StandardOutput.ReadToEnd();
        var stderr = proc.StandardError.ReadToEnd();
        proc.WaitForExit(120_000);

        return (proc.ExitCode, stdout + stderr);
    }

    private static void Check(bool cond, string name)
    {
        if (cond) { Console.WriteLine($"  [PASS] {name}"); ++_passed; }
        else      { Console.WriteLine($"  [FAIL] {name}"); ++_failed; }
    }
}
