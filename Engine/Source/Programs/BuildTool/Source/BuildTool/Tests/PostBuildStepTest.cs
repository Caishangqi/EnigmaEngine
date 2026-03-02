// Copyright EnigmaEngine. All Rights Reserved.

using System.Text.Json;
using BuildTool.Analysis;
using BuildTool.Build;
using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Tests for PostBuildStep: binary placement and manifest generation.
/// Uses temp directories with fake binaries - no real CMake builds required.
///
/// Development (Modular):
///   [1]  All DLLs copied flat to output dir
///   [2]  PDB files copied alongside DLLs
///   [3]  Host EXE copied to output dir
///   [4]  .modules JSON generated with BuildId + Modules map
///   [5]  .target JSON generated with LinkType="Modular"
///
/// Shipping (Monolithic):
///   [6]  Monolithic EXE copied to output dir
///   [7]  No DLL files in output dir
///   [8]  No .modules file generated
///   [9]  .target JSON has LinkType="Monolithic"
///
/// Error handling:
///   [10] Locked file produces clear error
///
/// Naming:
///   [11] DLL/EXE naming follows convention
/// </summary>
public static class PostBuildStepTest
{
    public static void Run()
    {
        Console.WriteLine("=== PostBuildStep Tests ===");
        Console.WriteLine();

        TestDevelopmentCopiesDlls();
        TestDevelopmentCopiesPdbs();
        TestDevelopmentCopiesExe();
        TestDevelopmentGeneratesModulesFile();
        TestDevelopmentGeneratesTargetFile();
        TestShippingCopiesExeOnly();
        TestShippingNoDlls();
        TestShippingNoModulesFile();
// PLACEHOLDER_MORE_TESTS
        TestShippingTargetHasMonolithicLinkType();
        TestHandlesLockedFile();
        TestNamingConventions();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    // ── Helpers ──────────────────────────────────────────────

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }

    private static JsonElement ParseJson(string json) =>
        JsonDocument.Parse(json).RootElement;

    /// <summary>
    /// Create a minimal ScanResult with Core (library), Engine (library), Launch (executable).
    /// </summary>
    private static ProjectScanner.ScanResult MakeScanResult(string projectRoot)
    {
        var allModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core" },
            },
            ["Launch"] = new()
            {
                ModuleName = "Launch",
                PublicDependencyModuleNames = { "Engine" },
            },
        };

        var gameTarget = new TargetRules
        {
            TargetName = "TestGame",
            Type = TargetType.Game,
            ExtraModuleNames = { "Launch" },
        };

        return new ProjectScanner.ScanResult
        {
            ProjectDescriptor = new ProjectDescriptor
            {
                FileVersion = 1,
                Modules = { new ModuleDescriptor { Name = "Launch" } },
            },
            ProjectName = "TestGame",
            EprojectPath = Path.Combine(projectRoot, "TestGame.eproject"),
            GameTarget = gameTarget,
            AllModules = allModules,
            EngineModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
            {
                ["Core"] = allModules["Core"],
                ["Engine"] = allModules["Engine"],
            },
            GameModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
            {
                ["Launch"] = allModules["Launch"],
            },
            ThirdPartyModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal),
            PluginScanResult = new PluginScanner.ScanResult(),
            ResolveResult = new DependencyResolver.ResolveResult
            {
                Success = true,
                BuildOrder = new[] { "Core", "Engine", "Launch" },
            },
            ProjectRoot = projectRoot,
            EngineRoot = Path.Combine(projectRoot, "..", "Engine"),
        };
    }

    /// <summary>
    /// Create a temp directory with fake CMake build output files.
    /// Simulates multi-config generator output (files in Release/ subdirectory).
    /// </summary>
    private static (string buildDir, string outputDir, string projectRoot) SetupTempDirs(
        BuildConfiguration config = BuildConfiguration.Development)
    {
        string root = Path.Combine(Path.GetTempPath(), $"PostBuildTest_{Guid.NewGuid():N}");
        string projectRoot = Path.Combine(root, "Project");
        string buildDir = Path.Combine(root, "Build", "Release");
        string outputDir = Path.Combine(projectRoot, "Binaries", "Win64");

        Directory.CreateDirectory(buildDir);
        Directory.CreateDirectory(projectRoot);

        // Create fake binaries based on config
        if (config == BuildConfiguration.Shipping)
        {
            CreateFakeFile(buildDir, "TestGame-Win64-Shipping.exe");
            CreateFakeFile(buildDir, "TestGame-Win64-Shipping.pdb");
        }
        else
        {
            // Development: EXE + module DLLs + PDBs
            CreateFakeFile(buildDir, "TestGame-Launch.exe");
            CreateFakeFile(buildDir, "TestGame-Launch.pdb");
            CreateFakeFile(buildDir, "TestGame-Core.dll");
            CreateFakeFile(buildDir, "TestGame-Core.pdb");
            CreateFakeFile(buildDir, "TestGame-Engine.dll");
            CreateFakeFile(buildDir, "TestGame-Engine.pdb");
        }

        return (Path.Combine(root, "Build"), outputDir, projectRoot);
    }

    private static void CreateFakeFile(string dir, string name)
    {
        File.WriteAllBytes(Path.Combine(dir, name), new byte[] { 0x4D, 0x5A }); // MZ header
    }

    private static void CleanupTempDir(string path)
    {
        try
        {
            string root = Path.GetDirectoryName(Path.GetDirectoryName(path)!)!;
            if (root.Contains("PostBuildTest_"))
                Directory.Delete(root, recursive: true);
            else
                Directory.Delete(path, recursive: true);
        }
        catch { /* Best-effort cleanup */ }
    }

    private static void CleanupRoot(string projectRoot)
    {
        try
        {
            // Navigate up to the PostBuildTest_ root
            string root = Path.GetDirectoryName(projectRoot)!;
            if (root.Contains("PostBuildTest_"))
                Directory.Delete(root, recursive: true);
        }
        catch { /* Best-effort cleanup */ }
    }

    private static PostBuildContext MakeContext(
        string buildDir, string outputDir, string projectRoot,
        BuildConfiguration config = BuildConfiguration.Development)
    {
        return new PostBuildContext
        {
            CmakeBuildDir = buildDir,
            OutputDir = outputDir,
            ProjectName = "TestGame",
            ScanResult = MakeScanResult(projectRoot),
            BuildOptions = new BuildOptions
            {
                ProjectPath = Path.Combine(projectRoot, "TestGame.eproject"),
                Configuration = config,
                Platform = "Win64",
            },
        };
    }
// PLACEHOLDER_TESTS

    // ── Test 1: Development copies DLLs flat ────────────────

    private static void TestDevelopmentCopiesDlls()
    {
        Console.WriteLine("[Test 1] Development: all DLLs copied flat to output dir");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");
            Assert(File.Exists(Path.Combine(outputDir, "TestGame-Core.dll")), "Core DLL in output");
            Assert(File.Exists(Path.Combine(outputDir, "TestGame-Engine.dll")), "Engine DLL in output");

            // Verify flat - no subdirectories
            var dirs = Directory.GetDirectories(outputDir);
            Assert(dirs.Length == 0, "Output dir should have no subdirectories");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // ── Test 2: Development copies PDBs ─────────────────────

    private static void TestDevelopmentCopiesPdbs()
    {
        Console.WriteLine("[Test 2] Development: PDB files copied alongside DLLs");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");
            Assert(File.Exists(Path.Combine(outputDir, "TestGame-Core.pdb")), "Core PDB");
            Assert(File.Exists(Path.Combine(outputDir, "TestGame-Engine.pdb")), "Engine PDB");
            Assert(File.Exists(Path.Combine(outputDir, "TestGame-Launch.pdb")), "Launch PDB");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // ── Test 3: Development copies host EXE ─────────────────

    private static void TestDevelopmentCopiesExe()
    {
        Console.WriteLine("[Test 3] Development: host EXE copied to output dir");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");
            Assert(File.Exists(Path.Combine(outputDir, "TestGame-Launch.exe")), "Launch EXE in output");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }
// PLACEHOLDER_TESTS_2

    // ── Test 4: Development generates .modules ──────────────

    private static void TestDevelopmentGeneratesModulesFile()
    {
        Console.WriteLine("[Test 4] Development: .modules JSON with BuildId + Modules map");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            var modulesPath = Path.Combine(projectRoot, "Binaries", "Win64", "TestGame.modules");
            Assert(File.Exists(modulesPath), ".modules file should exist");

            var json = ParseJson(File.ReadAllText(modulesPath));
            Assert(json.TryGetProperty("BuildId", out var bid) && bid.GetString()!.Length > 0,
                "BuildId should be non-empty");
            var mods = json.GetProperty("Modules");
            Assert(mods.TryGetProperty("Core", out _), "Modules should contain Core");
            Assert(mods.TryGetProperty("Engine", out _), "Modules should contain Engine");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // ── Test 5: Development generates .target ───────────────

    private static void TestDevelopmentGeneratesTargetFile()
    {
        Console.WriteLine("[Test 5] Development: .target JSON with LinkType=Modular");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            var targetPath = Path.Combine(projectRoot, "Binaries", "Win64", "TestGame.target");
            Assert(File.Exists(targetPath), ".target file should exist");

            var json = ParseJson(File.ReadAllText(targetPath));
            Assert(json.GetProperty("TargetName").GetString() == "TestGame", "TargetName");
            Assert(json.GetProperty("Platform").GetString() == "Win64", "Platform");
            Assert(json.GetProperty("Configuration").GetString() == "Development", "Configuration");
            Assert(json.GetProperty("LinkType").GetString() == "Modular", "LinkType should be Modular");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }
// PLACEHOLDER_TESTS_3

    // ── Test 6: Shipping copies EXE only ────────────────────

    private static void TestShippingCopiesExeOnly()
    {
        Console.WriteLine("[Test 6] Shipping: monolithic EXE copied to output dir");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot, BuildConfiguration.Shipping);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");
            Assert(File.Exists(Path.Combine(outputDir, "TestGame-Win64-Shipping.exe")),
                "Monolithic EXE in output");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // ── Test 7: Shipping has no DLLs ────────────────────────

    private static void TestShippingNoDlls()
    {
        Console.WriteLine("[Test 7] Shipping: no DLL files in output dir");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot, BuildConfiguration.Shipping);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            var dlls = Directory.GetFiles(outputDir, "*.dll");
            Assert(dlls.Length == 0, $"No DLLs expected, found {dlls.Length}");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // ── Test 8: Shipping has no .modules file ───────────────

    private static void TestShippingNoModulesFile()
    {
        Console.WriteLine("[Test 8] Shipping: no .modules file generated");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot, BuildConfiguration.Shipping);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            var modulesFiles = Directory.GetFiles(
                Path.Combine(projectRoot, "Binaries", "Win64"), "*.modules");
            Assert(modulesFiles.Length == 0,
                $"No .modules files expected, found {modulesFiles.Length}");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }
// PLACEHOLDER_TESTS_4

    // ── Test 9: Shipping .target has LinkType=Monolithic ────

    private static void TestShippingTargetHasMonolithicLinkType()
    {
        Console.WriteLine("[Test 9] Shipping: .target JSON has LinkType=Monolithic");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot, BuildConfiguration.Shipping);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            var targetPath = Path.Combine(
                projectRoot, "Binaries", "Win64", "TestGame-Win64-Shipping.target");
            Assert(File.Exists(targetPath), ".target file should exist");

            var json = ParseJson(File.ReadAllText(targetPath));
            Assert(json.GetProperty("LinkType").GetString() == "Monolithic",
                "LinkType should be Monolithic");
            Assert(json.GetProperty("Configuration").GetString() == "Shipping",
                "Configuration should be Shipping");

            // BuildProducts should contain only the EXE
            var products = json.GetProperty("BuildProducts");
            Assert(products.GetArrayLength() == 1, "Should have exactly 1 build product");
            Assert(products[0].GetProperty("Type").GetString() == "Executable",
                "Build product type should be Executable");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // ── Test 10: Handles locked file ────────────────────────

    private static void TestHandlesLockedFile()
    {
        Console.WriteLine("[Test 10] Locked file: returns clear error message");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs();
        try
        {
            // Pre-create output dir and lock a file that will be overwritten
            Directory.CreateDirectory(outputDir);
            string lockedPath = Path.Combine(outputDir, "TestGame-Core.dll");
            File.WriteAllBytes(lockedPath, new byte[] { 0x00 });

            // Hold an exclusive lock on the file
            using var lockStream = new FileStream(
                lockedPath, FileMode.Open, FileAccess.ReadWrite, FileShare.None);

            var ctx = MakeContext(buildDir, outputDir, projectRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(!result.Success, "Execute should fail when file is locked");
            Assert(result.Message.Contains("locked") || result.Message.Contains("failed"),
                $"Error should mention lock/failure: {result.Message}");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // ── Test 11: Naming conventions ─────────────────────────

    private static void TestNamingConventions()
    {
        Console.WriteLine("[Test 11] Naming: DLL/EXE follow {Project}-{Module} convention");
        var (buildDir, outputDir, projectRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, outputDir, projectRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            // Verify Development naming: {Project}-{Module}.dll (no platform/config suffix)
            var modulesPath = Path.Combine(projectRoot, "Binaries", "Win64", "TestGame.modules");
            var json = ParseJson(File.ReadAllText(modulesPath));
            var mods = json.GetProperty("Modules");

            var coreDll = mods.GetProperty("Core").GetString();
            Assert(coreDll == "TestGame-Core.dll",
                $"Core DLL should be TestGame-Core.dll, got {coreDll}");

            var engineDll = mods.GetProperty("Engine").GetString();
            Assert(engineDll == "TestGame-Engine.dll",
                $"Engine DLL should be TestGame-Engine.dll, got {engineDll}");

            // Verify .target Launch exe naming
            var targetPath = Path.Combine(projectRoot, "Binaries", "Win64", "TestGame.target");
            var targetJson = ParseJson(File.ReadAllText(targetPath));
            Assert(targetJson.GetProperty("Launch").GetString() == "TestGame.exe",
                "Development launch exe should be TestGame.exe");

            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }
}
