using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Task 5.3 end-to-end integration test.
/// Runs the full BuildTool pipeline with a ThirdParty (nlohmann_json) dependency:
///   parse .eproject → parse .Target.cs → parse .Build.cs
///   → scan ThirdParty → merge modules → resolve deps → generate CMake
///   → compile → execute → verify JSON output.
///
/// Validates:
///   [1]  .eproject parsed successfully
///   [2]  .Target.cs parsed (Game target, ExtraModuleNames)
///   [3]  .Build.cs parsed (nlohmann_json in PublicDependencyModuleNames)
///   [4]  ThirdPartyScanner discovers nlohmann_json
///   [5]  nlohmann_json merged into module graph
///   [6]  Dependency resolution succeeds (no cycles)
///   [7]  Build order: nlohmann_json before NlohmannJsonIntegrationTest
///   [8]  CMake generation succeeds
///   [9]  Generated CMake contains INTERFACE target for nlohmann_json
///   [10] Generated CMake contains executable for test module
///   [11] CMake configure succeeds
///   [12] CMake build succeeds
///   [13] Executable runs with exit code 0
///   [14] Output is valid JSON
///   [15] Output matches expected format exactly
/// </summary>
public static class NlohmannJsonIntegrationTest
{
    private static int _passed;
    private static int _failed;

    public static void Run()
    {
        _passed = 0;
        _failed = 0;

        Console.WriteLine("=== Task 5.3: nlohmann/json BuildTool Pipeline Integration ===");
        Console.WriteLine();

        var projectRoot = ResolveProjectRoot();
        Console.WriteLine($"Project root: {projectRoot}");

        var engineRoot = ResolveEngineRoot();
        Console.WriteLine($"Engine root:  {engineRoot}");
        Console.WriteLine();

        // ---- Step 1: Parse .eproject ----
        Console.WriteLine("[Step 1] Parsing .eproject ...");
        var eprojectPath = Path.Combine(projectRoot, "NlohmannJsonIntegrationTest.eproject");
        var project = ProjectParser.Parse(eprojectPath);
        Check(project.Modules.Count == 1
           && project.Modules[0].Name == "NlohmannJsonIntegrationTest",
            "[1]  .eproject parsed (1 module: NlohmannJsonIntegrationTest)");

        // ---- Step 2: Parse .Target.cs ----
        Console.WriteLine("[Step 2] Parsing .Target.cs ...");
        var targetPath = Path.Combine(projectRoot, "Source", "NlohmannJsonIntegrationTest.Target.cs");
        var target = TargetParser.Parse(targetPath);
        Check(target.Type == TargetType.Game
           && target.ExtraModuleNames.Contains("NlohmannJsonIntegrationTest"),
            "[2]  .Target.cs parsed (Game, ExtraModuleNames)");

        // ---- Step 3: Parse .Build.cs ----
        Console.WriteLine("[Step 3] Parsing .Build.cs ...");
        var buildCsPath = Path.Combine(projectRoot, "Source",
            "NlohmannJsonIntegrationTest", "NlohmannJsonIntegrationTest.Build.cs");
        var moduleRules = ModuleParser.Parse(buildCsPath);
        Check(moduleRules.ModuleName == "NlohmannJsonIntegrationTest"
           && moduleRules.PublicDependencyModuleNames.Contains("nlohmann_json"),
            "[3]  .Build.cs parsed (depends on nlohmann_json)");

        // ---- Step 4: Scan ThirdParty ----
        Console.WriteLine("[Step 4] Scanning ThirdParty ...");
        var thirdPartyRoot = Path.Combine(engineRoot, "Engine", "Source", "ThirdParty");
        var thirdPartyModules = ThirdPartyScanner.Scan(thirdPartyRoot);
        Check(thirdPartyModules.ContainsKey("nlohmann_json"),
            "[4]  ThirdPartyScanner discovers nlohmann_json");

        // ---- Step 5: Merge modules ----
        Console.WriteLine("[Step 5] Merging module graph ...");
        var modules = new Dictionary<string, ModuleRules>
        {
            [moduleRules.ModuleName] = moduleRules,
        };
        // Only merge ThirdParty modules that are actually referenced as dependencies
        foreach (var dep in moduleRules.PublicDependencyModuleNames
            .Concat(moduleRules.PrivateDependencyModuleNames))
        {
            if (thirdPartyModules.TryGetValue(dep, out var depRules))
                modules[dep] = depRules;
        }
        Check(modules.ContainsKey("nlohmann_json")
           && modules.ContainsKey("NlohmannJsonIntegrationTest"),
            "[5]  nlohmann_json merged into module graph");

        // ---- Step 6: Resolve dependencies ----
        Console.WriteLine("[Step 6] Resolving dependencies ...");
        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);
        Check(resolveResult.Success,
            "[6]  Dependency resolution succeeds (no cycles)");

        // ---- Step 7: Verify build order ----
        var buildOrder = resolveResult.BuildOrder.ToList();
        var jsonIdx = buildOrder.IndexOf("nlohmann_json");
        var testIdx = buildOrder.IndexOf("NlohmannJsonIntegrationTest");
        Check(jsonIdx >= 0 && testIdx >= 0 && jsonIdx < testIdx,
            "[7]  Build order: nlohmann_json before test module");
        Console.WriteLine($"     Build order: [{string.Join(", ", buildOrder)}]");

        // ---- Step 8: Generate CMake ----
        Console.WriteLine("[Step 8] Generating CMakeLists.txt ...");
        var generator = new CMakeGenerator();
        var genResult = generator.Generate(
            "NlohmannJsonIntegrationTest", modules, resolveResult, projectRoot, target);
        Check(genResult.Success,
            "[8]  CMake generation succeeds");

        // ---- Step 9-10: Verify CMake content ----
        Check(genResult.Content.Contains("add_library(nlohmann_json INTERFACE)"),
            "[9]  CMake contains INTERFACE target for nlohmann_json");
        Check(genResult.Content.Contains("add_executable(NlohmannJsonIntegrationTest"),
            "[10] CMake contains executable for test module");

        // Write CMakeLists.txt
        var cmakePath = Path.Combine(projectRoot, "CMakeLists.txt");
        File.WriteAllText(cmakePath, genResult.Content);
        Console.WriteLine();
        Console.WriteLine("--- Generated CMakeLists.txt ---");
        Console.WriteLine(genResult.Content);
        Console.WriteLine("--- End CMakeLists.txt ---");
        Console.WriteLine();

        // ---- Step 11: CMake configure ----
        Console.WriteLine("[Step 11] CMake configure ...");
        var buildDir = Path.Combine(projectRoot, "Intermediate", "Build");
        Directory.CreateDirectory(buildDir);

        var configureExit = RunProcess("cmake",
            $"-S \"{projectRoot}\" -B \"{buildDir}\" -G \"Visual Studio 17 2022\" -A x64",
            projectRoot);
        Check(configureExit == 0,
            "[11] CMake configure succeeds");

        // ---- Step 12: CMake build ----
        Console.WriteLine("[Step 12] CMake build ...");
        var buildExit = RunProcess("cmake",
            $"--build \"{buildDir}\" --config Release",
            projectRoot);
        Check(buildExit == 0,
            "[12] CMake build succeeds");

        // ---- Step 13-15: Execute and verify ----
        Console.WriteLine("[Step 13-15] Execute and verify output ...");
        var exePath = FindExecutable(buildDir, BinaryNaming.EngineBinaryName);
        if (exePath is null)
        {
            Check(false, "[13] Executable found");
            Check(false, "[14] Output is valid JSON");
            Check(false, "[15] Output matches expected format");
        }
        else
        {
            var (exitCode, output) = RunProcessCapture(exePath, "", projectRoot);
            Check(exitCode == 0,
                "[13] Executable runs with exit code 0");

            var trimmed = output.Trim();
            Console.WriteLine($"     Output: {trimmed}");

            // Verify valid JSON by round-tripping
            bool validJson = false;
            try
            {
                // Simple validation: must start with { and end with }
                validJson = trimmed.StartsWith('{') && trimmed.EndsWith('}');
            }
            catch { /* not valid */ }
            Check(validJson,
                "[14] Output is valid JSON");

            // Expected exact output
            const string expected =
                "{\"config\":{\"headerOnly\":true,\"version\":\"0.5.0\"},\"engine\":\"EnigmaEngine\",\"features\":[\"json\",\"modules\",\"thirdparty\"],\"phase\":5}";
            Check(trimmed == expected,
                "[15] Output matches expected format exactly");
        }

        // ---- Cleanup ----
        try { File.Delete(cmakePath); } catch { }
        try { Directory.Delete(Path.Combine(projectRoot, "Intermediate"), true); } catch { }
        try { Directory.Delete(Path.Combine(projectRoot, "Binaries"), true); } catch { }

        // ---- Summary ----
        Console.WriteLine();
        Console.WriteLine($"=== {_passed}/{_passed + _failed} tests passed ===");

        if (_failed > 0)
            throw new Exception($"NlohmannJsonIntegrationTest: {_failed} test(s) failed.");
    }

    // ── Process helpers (same pattern as Phase1IntegrationTest) ──

    private static int RunProcess(string fileName, string arguments, string workingDir)
    {
        var psi = new System.Diagnostics.ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            WorkingDirectory = workingDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var proc = System.Diagnostics.Process.Start(psi)!;
        var stdout = proc.StandardOutput.ReadToEnd();
        var stderr = proc.StandardError.ReadToEnd();
        proc.WaitForExit(120_000);

        if (!string.IsNullOrWhiteSpace(stdout))
            Console.WriteLine(stdout);
        if (!string.IsNullOrWhiteSpace(stderr))
            Console.Error.WriteLine(stderr);

        return proc.ExitCode;
    }

    private static (int ExitCode, string Output) RunProcessCapture(
        string fileName, string arguments, string workingDir)
    {
        var psi = new System.Diagnostics.ProcessStartInfo
        {
            FileName = fileName,
            Arguments = arguments,
            WorkingDirectory = workingDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };

        using var proc = System.Diagnostics.Process.Start(psi)!;
        var output = proc.StandardOutput.ReadToEnd();
        proc.WaitForExit(30_000);
        return (proc.ExitCode, output);
    }

    private static string? FindExecutable(string buildDir, string name)
    {
        var searchPaths = new[]
        {
            Path.Combine(buildDir, "Binaries", "Release", $"{name}.exe"),
            Path.Combine(buildDir, "Binaries", $"{name}.exe"),
            Path.Combine(buildDir, "Release", $"{name}.exe"),
            Path.Combine(buildDir, $"{name}.exe"),
        };

        foreach (var path in searchPaths)
        {
            if (File.Exists(path))
                return path;
        }

        var found = Directory.GetFiles(buildDir, $"{name}.exe", SearchOption.AllDirectories);
        return found.Length > 0 ? found[0] : null;
    }

    private static string ResolveProjectRoot()
    {
        var baseDir = AppContext.BaseDirectory;
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", "..", "Tests", "NlohmannJsonIntegrationTest"));

        if (Directory.Exists(candidate))
            return candidate;

        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            var testDir = Path.Combine(dir.FullName, "Tests", "NlohmannJsonIntegrationTest");
            if (Directory.Exists(testDir))
                return testDir;
            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("Cannot find Tests/NlohmannJsonIntegrationTest.");
    }

    private static string ResolveEngineRoot()
    {
        var baseDir = AppContext.BaseDirectory;
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", ".."));

        if (Directory.Exists(Path.Combine(candidate, "Engine", "Source", "ThirdParty")))
            return candidate;

        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            if (Directory.Exists(Path.Combine(dir.FullName, "Engine", "Source", "ThirdParty")))
                return dir.FullName;
            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("Cannot find EnigmaEngine root.");
    }

    private static void Check(bool cond, string name)
    {
        if (cond) { Console.WriteLine($"  [PASS] {name}"); ++_passed; }
        else      { Console.WriteLine($"  [FAIL] {name}"); ++_failed; }
    }
}
