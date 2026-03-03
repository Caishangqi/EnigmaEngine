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
///   [1]  Engine DLLs copied to Engine/Binaries/Win64/
///   [2]  Game DLLs + EXE copied to Project/Binaries/Win64/
///   [3]  PDB files follow their DLL/EXE to correct directory
///   [4]  Engine .modules generated in Engine/Binaries/Win64/
///   [5]  Game .modules generated in Project/Binaries/Win64/
///   [6]  .target JSON generated with LinkType="Modular"
///
/// Shipping (Monolithic):
///   [7]  Monolithic EXE copied to Project/Binaries/Win64/
///   [8]  No DLL files in any output dir
///   [9]  No .modules file generated
///   [10] .target JSON has LinkType="Monolithic"
///
/// Error handling:
///   [11] Locked file produces clear error
/// </summary>
public static class PostBuildStepTest
{
    public static void Run()
    {
        Console.WriteLine("=== PostBuildStep Tests ===");
        Console.WriteLine();

        TestDevelopmentEngineModulesToEngineDir();
        TestDevelopmentGameModulesToGameDir();
        TestDevelopmentPdbsFollowBinaries();
        TestDevelopmentEngineModulesManifest();
        TestDevelopmentGameModulesManifest();
        TestDevelopmentGeneratesTargetFile();
        TestShippingCopiesExeOnly();
        TestShippingNoDlls();
        TestShippingNoModulesFile();
        TestShippingTargetHasMonolithicLinkType();
        TestHandlesLockedFile();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    // -- Helpers -----------------------------------------------

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }

    private static JsonElement ParseJson(string json) =>
        JsonDocument.Parse(json).RootElement;

    /// <summary>
    /// Create a minimal ScanResult with Core + Engine + Launch (engine),
    /// and GamePlay (game module).
    /// </summary>
    private static ProjectScanner.ScanResult MakeScanResult(
        string projectRoot, string engineRoot)
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
            ["GamePlay"] = new()
            {
                ModuleName = "GamePlay",
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
                Modules = { new ModuleDescriptor { Name = "GamePlay" } },
            },
            ProjectName = "TestGame",
            EprojectPath = Path.Combine(projectRoot, "TestGame.eproject"),
            GameTarget = gameTarget,
            AllModules = allModules,
            EngineModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
            {
                ["Core"] = allModules["Core"],
                ["Engine"] = allModules["Engine"],
                ["Launch"] = allModules["Launch"],
            },
            GameModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
            {
                ["GamePlay"] = allModules["GamePlay"],
            },
            ThirdPartyModules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal),
            PluginScanResult = new PluginScanner.ScanResult(),
            ResolveResult = new DependencyResolver.ResolveResult
            {
                Success = true,
                BuildOrder = new[] { "Core", "Engine", "Launch", "GamePlay" },
            },
            ProjectRoot = projectRoot,
            EngineRoot = engineRoot,
        };
    }

    /// <summary>
    /// Create temp directories with fake CMake build output files.
    /// </summary>
    private static (string buildDir, string projectRoot, string engineRoot) SetupTempDirs(
        BuildConfiguration config = BuildConfiguration.Development)
    {
        string root = Path.Combine(
            Path.GetTempPath(), $"PostBuildTest_{Guid.NewGuid():N}");
        string projectRoot = Path.Combine(root, "Project");
        string engineRoot = Path.Combine(root, "Engine");
        string buildDir = Path.Combine(root, "Build", "Release");

        Directory.CreateDirectory(buildDir);
        Directory.CreateDirectory(projectRoot);
        Directory.CreateDirectory(engineRoot);

        if (config == BuildConfiguration.Shipping)
        {
            CreateFakeFile(buildDir, "TestGame-Win64-Shipping.exe");
            CreateFakeFile(buildDir, "TestGame-Win64-Shipping.pdb");
        }
        else
        {
            // Engine modules
            CreateFakeFile(buildDir, "TestGame-Core.dll");
            CreateFakeFile(buildDir, "TestGame-Core.pdb");
            CreateFakeFile(buildDir, "TestGame-Engine.dll");
            CreateFakeFile(buildDir, "TestGame-Engine.pdb");
            CreateFakeFile(buildDir, "TestGame-Launch.dll");
            CreateFakeFile(buildDir, "TestGame-Launch.pdb");
            // Game modules
            CreateFakeFile(buildDir, "TestGame-GamePlay.dll");
            CreateFakeFile(buildDir, "TestGame-GamePlay.pdb");
            // EXE
            CreateFakeFile(buildDir, "TestGame.exe");
            CreateFakeFile(buildDir, "TestGame.pdb");
        }
        return (buildDir, projectRoot, engineRoot);
    }

    private static void CreateFakeFile(string dir, string name)
    {
        File.WriteAllBytes(Path.Combine(dir, name), new byte[] { 0xDE, 0xAD });
    }

    private static void CleanupRoot(string projectRoot)
    {
        try
        {
            string root = Path.GetDirectoryName(projectRoot)!;
            if (root.Contains("PostBuildTest_"))
                Directory.Delete(root, recursive: true);
        }
        catch { /* Best-effort cleanup */ }
    }

    private static PostBuildContext MakeContext(
        string buildDir, string projectRoot, string engineRoot,
        BuildConfiguration config = BuildConfiguration.Development)
    {
        return new PostBuildContext
        {
            CmakeBuildDir = buildDir,
            ProjectName = "TestGame",
            ScanResult = MakeScanResult(projectRoot, engineRoot),
            BuildOptions = new BuildOptions
            {
                ProjectPath = Path.Combine(projectRoot, "TestGame.eproject"),
                Configuration = config,
                Platform = "Win64",
            },
        };
    }

    // -- Test 1: Engine DLLs go to Engine/Binaries/ ------------

    private static void TestDevelopmentEngineModulesToEngineDir()
    {
        Console.WriteLine("[Test 1] Development: engine DLLs copied to Engine/Binaries/Win64/");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            string engineBin = Path.Combine(engineRoot, "Binaries", "Win64");
            Assert(File.Exists(Path.Combine(engineBin, "TestGame-Core.dll")), "Core DLL in Engine");
            Assert(File.Exists(Path.Combine(engineBin, "TestGame-Engine.dll")), "Engine DLL in Engine");
            Assert(File.Exists(Path.Combine(engineBin, "TestGame-Launch.dll")), "Launch DLL in Engine");

            string gameBin = Path.Combine(projectRoot, "Binaries", "Win64");
            Assert(!File.Exists(Path.Combine(gameBin, "TestGame-Core.dll")), "Core DLL NOT in Game");
            Assert(!File.Exists(Path.Combine(gameBin, "TestGame-Engine.dll")), "Engine DLL NOT in Game");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 2: Game DLLs + EXE go to Project/Binaries/ ------

    private static void TestDevelopmentGameModulesToGameDir()
    {
        Console.WriteLine("[Test 2] Development: game DLLs to Project/Binaries/Win64/, EXE to Engine/Binaries/Win64/");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            string gameBin = Path.Combine(projectRoot, "Binaries", "Win64");
            Assert(File.Exists(Path.Combine(gameBin, "TestGame-GamePlay.dll")), "GamePlay DLL in Game");

            // EXE goes to Engine/Binaries/ in Modular builds (alongside engine DLLs)
            string engineBin = Path.Combine(engineRoot, "Binaries", "Win64");
            Assert(File.Exists(Path.Combine(engineBin, "TestGame.exe")), "EXE in Engine");
            Assert(!File.Exists(Path.Combine(gameBin, "TestGame.exe")), "EXE NOT in Game");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 3: PDBs follow their binaries --------------------

    private static void TestDevelopmentPdbsFollowBinaries()
    {
        Console.WriteLine("[Test 3] Development: PDB files follow DLLs to correct directory");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            string engineBin = Path.Combine(engineRoot, "Binaries", "Win64");
            string gameBin = Path.Combine(projectRoot, "Binaries", "Win64");
            Assert(File.Exists(Path.Combine(engineBin, "TestGame-Core.pdb")), "Core PDB in Engine");
            Assert(File.Exists(Path.Combine(engineBin, "TestGame-Engine.pdb")), "Engine PDB in Engine");
            Assert(File.Exists(Path.Combine(gameBin, "TestGame-GamePlay.pdb")), "GamePlay PDB in Game");
            Assert(File.Exists(Path.Combine(engineBin, "TestGame.pdb")), "EXE PDB in Engine");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 4: Engine .modules in Engine/Binaries/ -----------

    private static void TestDevelopmentEngineModulesManifest()
    {
        Console.WriteLine("[Test 4] Development: engine .modules in Engine/Binaries/Win64/");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            var modulesPath = Path.Combine(engineRoot, "Binaries", "Win64", "TestGame.modules");
            Assert(File.Exists(modulesPath), "Engine .modules should exist");

            var json = ParseJson(File.ReadAllText(modulesPath));
            var mods = json.GetProperty("Modules");
            Assert(mods.TryGetProperty("Core", out _), "Engine .modules should contain Core");
            Assert(mods.TryGetProperty("Engine", out _), "Engine .modules should contain Engine");
            Assert(mods.TryGetProperty("Launch", out _), "Engine .modules should contain Launch");
            Assert(!mods.TryGetProperty("GamePlay", out _), "Engine .modules should NOT contain GamePlay");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 5: Game .modules in Project/Binaries/ ------------

    private static void TestDevelopmentGameModulesManifest()
    {
        Console.WriteLine("[Test 5] Development: game .modules in Project/Binaries/Win64/");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            var modulesPath = Path.Combine(projectRoot, "Binaries", "Win64", "TestGame.modules");
            Assert(File.Exists(modulesPath), "Game .modules should exist");

            var json = ParseJson(File.ReadAllText(modulesPath));
            var mods = json.GetProperty("Modules");
            Assert(mods.TryGetProperty("GamePlay", out _), "Game .modules should contain GamePlay");
            Assert(!mods.TryGetProperty("Core", out _), "Game .modules should NOT contain Core");
            Assert(!mods.TryGetProperty("Engine", out _), "Game .modules should NOT contain Engine");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 6: .target file generated ------------------------

    private static void TestDevelopmentGeneratesTargetFile()
    {
        Console.WriteLine("[Test 6] Development: .target JSON with LinkType=Modular");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs();
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot);
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

    // -- Test 7: Shipping copies EXE only ----------------------

    private static void TestShippingCopiesExeOnly()
    {
        Console.WriteLine("[Test 7] Shipping: monolithic EXE copied to Project/Binaries/Win64/");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot, BuildConfiguration.Shipping);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            string gameBin = Path.Combine(projectRoot, "Binaries", "Win64");
            Assert(File.Exists(Path.Combine(gameBin, "TestGame-Win64-Shipping.exe")),
                "Monolithic EXE in output");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 8: Shipping has no DLLs --------------------------

    private static void TestShippingNoDlls()
    {
        Console.WriteLine("[Test 8] Shipping: no DLL files in output dir");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot, BuildConfiguration.Shipping);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            string gameBin = Path.Combine(projectRoot, "Binaries", "Win64");
            var dlls = Directory.GetFiles(gameBin, "*.dll");
            Assert(dlls.Length == 0, $"No DLLs expected, found {dlls.Length}");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 9: Shipping has no .modules file -----------------

    private static void TestShippingNoModulesFile()
    {
        Console.WriteLine("[Test 9] Shipping: no .modules file generated");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot, BuildConfiguration.Shipping);
            var result = PostBuildStep.Execute(ctx);
            Assert(result.Success, $"Execute should succeed: {result.ErrorDetail}");

            string gameBin = Path.Combine(projectRoot, "Binaries", "Win64");
            var modulesFiles = Directory.GetFiles(gameBin, "*.modules");
            Assert(modulesFiles.Length == 0,
                $"No .modules files expected in game dir, found {modulesFiles.Length}");

            string engineBin = Path.Combine(engineRoot, "Binaries", "Win64");
            if (Directory.Exists(engineBin))
            {
                var engineModules = Directory.GetFiles(engineBin, "*.modules");
                Assert(engineModules.Length == 0,
                    $"No .modules files expected in engine dir, found {engineModules.Length}");
            }
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 10: Shipping .target has LinkType=Monolithic -----

    private static void TestShippingTargetHasMonolithicLinkType()
    {
        Console.WriteLine("[Test 10] Shipping: .target JSON has LinkType=Monolithic");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs(BuildConfiguration.Shipping);
        try
        {
            var ctx = MakeContext(buildDir, projectRoot, engineRoot, BuildConfiguration.Shipping);
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

            var products = json.GetProperty("BuildProducts");
            Assert(products.GetArrayLength() == 1, "Should have exactly 1 build product");
            Assert(products[0].GetProperty("Type").GetString() == "Executable",
                "Build product type should be Executable");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }

    // -- Test 11: Handles locked file --------------------------

    private static void TestHandlesLockedFile()
    {
        Console.WriteLine("[Test 11] Locked file: returns clear error message");
        var (buildDir, projectRoot, engineRoot) = SetupTempDirs();
        try
        {
            // Pre-create engine output dir and lock a file that will be overwritten
            string engineBin = Path.Combine(engineRoot, "Binaries", "Win64");
            Directory.CreateDirectory(engineBin);
            string lockedPath = Path.Combine(engineBin, "TestGame-Core.dll");
            File.WriteAllBytes(lockedPath, new byte[] { 0x00 });

            // Hold an exclusive lock on the file
            using var lockStream = new FileStream(
                lockedPath, FileMode.Open, FileAccess.ReadWrite, FileShare.None);

            var ctx = MakeContext(buildDir, projectRoot, engineRoot);
            var result = PostBuildStep.Execute(ctx);

            Assert(!result.Success, "Execute should fail when file is locked");
            Assert(result.Message.Contains("locked") || result.Message.Contains("failed"),
                $"Error should mention lock/failure: {result.Message}");
            Console.WriteLine("  PASSED");
        }
        finally { CleanupRoot(projectRoot); }
    }
}
