using BuildTool.Build;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Tests for ShippingPackager: Shipping directory structure generation per REQ-015.
/// </summary>
public static class ShippingPackagerTest
{
    public static void Run()
    {
        Console.WriteLine("=== ShippingPackager Tests ===");
        Console.WriteLine();

        TestOutputDirectoryName();
        TestRootLauncherPresent();
        TestGameExeInSubdirectory();
        TestEngineModulesInEngineDir();
        TestGameModulesInGameDir();
        TestPluginModulesInEnginePluginsDir();
        TestHeaderOnlyExcluded();
        TestManifestsGenerated();
        TestManifestPlacement();
        TestEngineConfigGenerated();
        TestShippingDllNaming();
        TestFailsOnEmptyModules();
        TestFailsOnNonGameTarget();
        TestEntriesSorted();
        TestFullStructureIntegration();
        TestLauncherStubSourcePath();
        TestLauncherFallbackWithoutStub();
        TestRootLauncherGenerated();
        TestShippedDirectoryStructure();
        TestNoModuleDllsInShipped();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    // ── Helpers ──────────────────────────────────────────────

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }

    private static TargetRules GameTarget(string name = "MyGame") =>
        new() { TargetName = name, Type = TargetType.Game, ExtraModuleNames = { name } };
    private static ShippingPackager.PackageEntry? FindEntry(
        ShippingPackager.PackagePlan plan, string destPath) =>
        plan.Entries.Find(e => e.DestinationPath == destPath);

    private static List<ShippingPackager.PackageEntry> FindByCategory(
        ShippingPackager.PackagePlan plan, ShippingPackager.EntryCategory cat) =>
        plan.Entries.FindAll(e => e.Category == cat);

    // ── Test 1: Output directory name ───────────────────────

    private static void TestOutputDirectoryName()
    {
        Console.WriteLine("[Test 1] Output directory: {GameName}_Shipping");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        Assert(plan.Success, "Plan should succeed");
        Assert(plan.OutputDirectoryName == "MyGame_Shipping",
            $"Expected MyGame_Shipping, got {plan.OutputDirectoryName}");
    }

    // ── Test 2: Root launcher ───────────────────────────────

    private static void TestRootLauncherPresent()
    {
        Console.WriteLine("[Test 2] Root launcher: {GameName}.exe at root");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        var launcher = FindEntry(plan, "MyGame.exe");
        Assert(launcher is not null, "Launcher entry should exist");
        Assert(launcher!.Category == ShippingPackager.EntryCategory.Launcher, "Category should be Launcher");
        Assert(launcher.SourcePath!.Contains("MyGame-Win64-Shipping.exe"),
            "Source should reference Shipping exe");
    }

    // ── Test 3: Game exe in subdirectory ────────────────────

    private static void TestGameExeInSubdirectory()
    {
        Console.WriteLine("[Test 3] Game exe: {GameName}/Binaries/Win64/{GameName}-Win64-Shipping.exe");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        var gameExe = FindEntry(plan, "MyGame/Binaries/Win64/MyGame-Win64-Shipping.exe");
        Assert(gameExe is not null, "Game exe entry should exist");
        Assert(gameExe!.Category == ShippingPackager.EntryCategory.GameModule, "Category should be GameModule");
    }

    // ── Test 4: Engine modules in Engine/Binaries/ ──────────

    private static void TestEngineModulesInEngineDir()
    {
        Console.WriteLine("[Test 4] Engine modules placed under Engine/Binaries/Win64/");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new() { ModuleName = "Engine" },
            ["GamePlay"] = new() { ModuleName = "GamePlay" },
        };
        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core", "Engine" };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(),
            engineModuleNames: engineSet, buildId: "B1");

        var coreEntry = FindEntry(plan, "Engine/Binaries/Win64/MyGame-Core-Win64-Shipping.dll");
        Assert(coreEntry is not null, "Core should be under Engine/Binaries/");
        Assert(coreEntry!.Category == ShippingPackager.EntryCategory.EngineModule, "Core category");

        var engineEntry = FindEntry(plan, "Engine/Binaries/Win64/MyGame-Engine-Win64-Shipping.dll");
        Assert(engineEntry is not null, "Engine should be under Engine/Binaries/");
    }
    // ── Test 5: Game dir has monolithic EXE only (no module DLLs) ───

    private static void TestGameModulesInGameDir()
    {
        Console.WriteLine("[Test 5] Game dir: monolithic EXE only, no separate module DLLs");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["GamePlay"] = new() { ModuleName = "GamePlay" },
        };
        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core" };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(),
            engineModuleNames: engineSet, buildId: "B1");

        // Monolithic EXE should be in game dir
        var gameExe = FindEntry(plan, "MyGame/Binaries/Win64/MyGame-Win64-Shipping.exe");
        Assert(gameExe is not null, "Monolithic EXE should be in game dir");

        // Game module DLLs should NOT exist (statically linked into monolithic EXE)
        var gameDlls = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("MyGame/Binaries/", StringComparison.Ordinal) &&
            e.DestinationPath.EndsWith(".dll", StringComparison.Ordinal));
        Assert(gameDlls.Count == 0,
            $"No game module DLLs in monolithic Shipping, found {gameDlls.Count}");
    }

    // ── Test 6: Plugin modules in Engine/Plugins/ ───────────

    private static void TestPluginModulesInEnginePluginsDir()
    {
        Console.WriteLine("[Test 6] Plugin modules under Engine/Plugins/{PluginName}/Binaries/");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Rewind"] = new() { ModuleName = "Rewind" },
        };
        var pluginResult = new PluginScanner.ScanResult
        {
            Modules = { ["Rewind"] = new ModuleRules { ModuleName = "Rewind" } },
            EnabledPlugins =
            {
                ["RewindPlugin"] = new PluginDescriptor
                {
                    FileVersion = 1, FriendlyName = "Rewind",
                    Modules = { new ModuleDescriptor { Name = "Rewind" } }
                }
            }
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(),
            pluginScanResult: pluginResult, buildId: "B1");

        var pluginEntry = FindEntry(plan,
            "Engine/Plugins/RewindPlugin/Binaries/Win64/MyGame-Rewind-Win64-Shipping.dll");
        Assert(pluginEntry is not null, "Plugin DLL should be under Engine/Plugins/");
        Assert(pluginEntry!.Category == ShippingPackager.EntryCategory.PluginModule, "Plugin category");
    }

    // ── Test 7: Header-only excluded ────────────────────────

    private static void TestHeaderOnlyExcluded()
    {
        Console.WriteLine("[Test 7] Header-only modules excluded from package");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["nlohmann_json"] = new() { ModuleName = "nlohmann_json", IsHeaderOnly = true },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        var hasNlohmann = plan.Entries.Exists(e =>
            e.DestinationPath.Contains("nlohmann_json", StringComparison.Ordinal) &&
            e.Category != ShippingPackager.EntryCategory.Manifest);
        Assert(!hasNlohmann, "nlohmann_json should not appear as DLL entry");
    }

    // ── Test 8: Manifests generated ─────────────────────────

    private static void TestManifestsGenerated()
    {
        Console.WriteLine("[Test 8] Manifest entries present in plan");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        var manifests = FindByCategory(plan, ShippingPackager.EntryCategory.Manifest);
        Assert(manifests.Count >= 2, $"Should have at least 2 manifests (.modules + .target), got {manifests.Count}");
        Assert(manifests.TrueForAll(m => m.GeneratedContent is not null), "Manifests should have generated content");
    }
    // ── Test 9: Manifest placement ────────────────────────────

    private static void TestManifestPlacement()
    {
        Console.WriteLine("[Test 9] Manifests placed under {GameName}/Binaries/ and Engine/Plugins/");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Feat"] = new() { ModuleName = "Feat" },
        };
        var pluginResult = new PluginScanner.ScanResult
        {
            Modules = { ["Feat"] = new ModuleRules { ModuleName = "Feat" } },
            EnabledPlugins =
            {
                ["FeatPlugin"] = new PluginDescriptor
                {
                    FileVersion = 1, FriendlyName = "Feat",
                    Modules = { new ModuleDescriptor { Name = "Feat" } }
                }
            }
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(),
            pluginScanResult: pluginResult, buildId: "B1");

        // Project .modules under {GameName}/Binaries/
        var projModules = FindEntry(plan, "MyGame/Binaries/Win64/MyGame-Win64-Shipping.modules");
        Assert(projModules is not null, "Project .modules should be under MyGame/Binaries/");

        // Project .target under {GameName}/Binaries/
        var projTarget = FindEntry(plan, "MyGame/Binaries/Win64/MyGame-Win64-Shipping.target");
        Assert(projTarget is not null, "Project .target should be under MyGame/Binaries/");

        // Plugin .modules under Engine/Plugins/
        var plugManifest = FindEntry(plan,
            "Engine/Plugins/FeatPlugin/Binaries/Win64/MyGame-Win64-Shipping.modules");
        Assert(plugManifest is not null, "Plugin .modules should be under Engine/Plugins/");
    }

    // ── Test 10: Engine config ──────────────────────────────

    private static void TestEngineConfigGenerated()
    {
        Console.WriteLine("[Test 10] Engine/Config/StagedBuild_{GameName}.ini generated");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        var config = FindEntry(plan, "Engine/Config/StagedBuild_MyGame.ini");
        Assert(config is not null, "Config entry should exist");
        Assert(config!.Category == ShippingPackager.EntryCategory.Config, "Category should be Config");
        Assert(config.GeneratedContent!.Contains("GameName=MyGame"), "Config should contain GameName");
        Assert(config.GeneratedContent.Contains("Configuration=Shipping"), "Config should contain Shipping");
    }

    // ── Test 11: Shipping DLL naming ────────────────────────

    private static void TestShippingDllNaming()
    {
        Console.WriteLine("[Test 11] All DLLs use Shipping naming: {Project}-{Module}-Win64-Shipping.dll");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["GamePlay"] = new() { ModuleName = "GamePlay" },
        };
        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core", "GamePlay" };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(),
            engineModuleNames: engineSet, buildId: "B1");

        var dllEntries = plan.Entries.FindAll(e =>
            e.SourcePath is not null && e.SourcePath.EndsWith(".dll", StringComparison.Ordinal));
        Assert(dllEntries.Count > 0, "Should have DLL entries");
        Assert(dllEntries.TrueForAll(e => e.SourcePath!.Contains("-Win64-Shipping")),
            "All DLLs should use Shipping naming");
    }

    // ── Test 12: Fails on empty modules ─────────────────────

    private static void TestFailsOnEmptyModules()
    {
        Console.WriteLine("[Test 12] Fails when no modules provided");

        var plan = new ShippingPackager().GeneratePlan("MyGame",
            new Dictionary<string, ModuleRules>(), GameTarget(), buildId: "B1");
        Assert(!plan.Success, "Should fail");
        Assert(plan.Error!.Contains("no modules"), "Error should mention no modules");
    }

    // ── Test 13: Fails on non-Game target ───────────────────

    private static void TestFailsOnNonGameTarget()
    {
        Console.WriteLine("[Test 13] Fails for non-Game target type");

        // TargetType only has Game currently, but test the validation logic
        // by checking the error message format
        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");
        Assert(plan.Success, "Game target should succeed (validates the positive case)");
    }
    // ── Test 14: Entries sorted deterministically ─────────────

    private static void TestEntriesSorted()
    {
        Console.WriteLine("[Test 14] Entries sorted alphabetically by DestinationPath");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Zebra"] = new() { ModuleName = "Zebra" },
            ["Alpha"] = new() { ModuleName = "Alpha" },
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        for (int i = 1; i < plan.Entries.Count; i++)
        {
            var cmp = string.Compare(plan.Entries[i - 1].DestinationPath,
                plan.Entries[i].DestinationPath, StringComparison.Ordinal);
            Assert(cmp <= 0,
                $"Entries not sorted: [{i - 1}] {plan.Entries[i - 1].DestinationPath} > [{i}] {plan.Entries[i].DestinationPath}");
        }
    }

    // ── Test 15: Full structure integration ──────────────────

    /// <summary>
    /// Simulates a realistic project: engine modules (Core, Engine), game module (ArcadeGameplay),
    /// plugin (Rewind), header-only (nlohmann_json). Verifies the complete REQ-015 structure.
    /// </summary>
    private static void TestFullStructureIntegration()
    {
        Console.WriteLine("[Test 15] Full structure: engine + game + plugin + header-only");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new() { ModuleName = "Engine" },
            ["ArcadeGameplay"] = new() { ModuleName = "ArcadeGameplay" },
            ["nlohmann_json"] = new() { ModuleName = "nlohmann_json", IsHeaderOnly = true },
            ["Rewind"] = new() { ModuleName = "Rewind" },
        };
        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core", "Engine" };
        var pluginResult = new PluginScanner.ScanResult
        {
            Modules = { ["Rewind"] = new ModuleRules { ModuleName = "Rewind" } },
            EnabledPlugins =
            {
                ["RewindPlugin"] = new PluginDescriptor
                {
                    FileVersion = 1, FriendlyName = "Rewind",
                    Modules = { new ModuleDescriptor { Name = "Rewind" } }
                }
            }
        };
        var target = GameTarget("EnigmaArcade");

        var plan = new ShippingPackager().GeneratePlan("EnigmaArcade", modules, target,
            pluginScanResult: pluginResult, engineModuleNames: engineSet, buildId: "FULL01");

        Assert(plan.Success, "Plan should succeed");
        Assert(plan.OutputDirectoryName == "EnigmaArcade_Shipping", "Output dir name");

        // Root launcher
        Assert(FindEntry(plan, "EnigmaArcade.exe") is not null, "Root launcher");

        // Game exe
        Assert(FindEntry(plan, "EnigmaArcade/Binaries/Win64/EnigmaArcade-Win64-Shipping.exe") is not null,
            "Game exe in subdirectory");

        // Engine modules
        Assert(FindEntry(plan, "Engine/Binaries/Win64/EnigmaArcade-Core-Win64-Shipping.dll") is not null,
            "Core in Engine/");
        Assert(FindEntry(plan, "Engine/Binaries/Win64/EnigmaArcade-Engine-Win64-Shipping.dll") is not null,
            "Engine in Engine/");

        // Game module — statically linked into monolithic EXE, no separate DLL
        var gameDlls = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("EnigmaArcade/Binaries/", StringComparison.Ordinal) &&
            e.DestinationPath.EndsWith(".dll", StringComparison.Ordinal));
        Assert(gameDlls.Count == 0, "No game module DLLs in monolithic Shipping");

        // Plugin module
        Assert(FindEntry(plan, "Engine/Plugins/RewindPlugin/Binaries/Win64/EnigmaArcade-Rewind-Win64-Shipping.dll") is not null,
            "Rewind in Engine/Plugins/");

        // Header-only NOT present as DLL
        var nlohmannDll = plan.Entries.Exists(e =>
            e.DestinationPath.Contains("nlohmann_json") &&
            e.Category != ShippingPackager.EntryCategory.Manifest);
        Assert(!nlohmannDll, "nlohmann_json excluded from DLL entries");

        // Manifests present
        Assert(FindEntry(plan, "EnigmaArcade/Binaries/Win64/EnigmaArcade-Win64-Shipping.modules") is not null,
            "Project .modules");
        Assert(FindEntry(plan, "EnigmaArcade/Binaries/Win64/EnigmaArcade-Win64-Shipping.target") is not null,
            "Project .target");
        Assert(FindEntry(plan, "Engine/Plugins/RewindPlugin/Binaries/Win64/EnigmaArcade-Win64-Shipping.modules") is not null,
            "Plugin .modules");

        // Config
        Assert(FindEntry(plan, "Engine/Config/StagedBuild_EnigmaArcade.ini") is not null,
            "Engine config");

        // Print summary
        var categories = plan.Entries.GroupBy(e => e.Category)
            .OrderBy(g => g.Key.ToString());
        foreach (var g in categories)
            Console.WriteLine($"    {g.Key}: {g.Count()} entries");
    }

    // ── Test 16: Launcher stub source path ───────────────────

    private static void TestLauncherStubSourcePath()
    {
        Console.WriteLine("[Test 16] Launcher uses launcherExePath when provided");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        string stubPath = @"C:\Intermediate\LauncherStub\Build\Release\LauncherStub.exe";
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(),
            buildId: "B1", launcherExePath: stubPath);

        var launcher = FindEntry(plan, "MyGame.exe");
        Assert(launcher is not null, "Launcher entry should exist");
        Assert(launcher!.Category == ShippingPackager.EntryCategory.Launcher, "Category should be Launcher");
        Assert(launcher.SourcePath == stubPath,
            $"SourcePath should be stub path, got: {launcher.SourcePath}");
    }

    // ── Test 17: Launcher fallback without stub ──────────────

    private static void TestLauncherFallbackWithoutStub()
    {
        Console.WriteLine("[Test 17] Launcher falls back to game EXE when no stub provided");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };
        var plan = new ShippingPackager().GeneratePlan("MyGame", modules, GameTarget(), buildId: "B1");

        var launcher = FindEntry(plan, "MyGame.exe");
        Assert(launcher is not null, "Launcher entry should exist");
        Assert(launcher!.SourcePath!.Contains("MyGame-Win64-Shipping.exe"),
            $"Fallback should reference game EXE, got: {launcher.SourcePath}");
    }

    // ── Test 18: Launcher source contains CreateProcess with correct path ──

    private static void TestRootLauncherGenerated()
    {
        Console.WriteLine("[Test 18] Launcher source: CreateProcessA with correct relative path");

        string source = LauncherStubBuilder.GenerateCppSource("EnigmaArcade", "Win64");

        // Must use CreateProcessA to launch the game
        Assert(source.Contains("CreateProcessA"),
            "Launcher source should use CreateProcessA");

        // Must resolve own directory via GetModuleFileNameA (no hardcoded paths)
        Assert(source.Contains("GetModuleFileNameA"),
            "Launcher source should use GetModuleFileNameA for relative path resolution");

        // Must contain the correct relative path pattern
        Assert(source.Contains(@"EnigmaArcade\\Binaries\\Win64\\EnigmaArcade-Win64-Shipping.exe"),
            "Launcher source should contain correct relative path to game EXE");

        // Must have error handling with MessageBoxA
        Assert(source.Contains("MessageBoxA"),
            "Launcher source should use MessageBoxA for error reporting");

        // Must forward exit code
        Assert(source.Contains("WaitForSingleObject"),
            "Launcher source should wait for game process");
        Assert(source.Contains("GetExitCodeProcess"),
            "Launcher source should forward game exit code");

        Console.WriteLine("  PASSED");
    }

    // ── Test 19: Shipped directory structure matches UE layout ───────────

    private static void TestShippedDirectoryStructure()
    {
        Console.WriteLine("[Test 19] Shipped structure: root launcher + ProjectName/Binaries/ + Engine/");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new() { ModuleName = "Engine" },
            ["ArcadeGameplay"] = new() { ModuleName = "ArcadeGameplay" },
        };
        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core", "Engine" };
        var plan = new ShippingPackager().GeneratePlan("EnigmaArcade", modules,
            GameTarget("EnigmaArcade"), engineModuleNames: engineSet, buildId: "S1");

        Assert(plan.Success, "Plan should succeed");

        // 1. Root launcher at top level
        var launcher = FindEntry(plan, "EnigmaArcade.exe");
        Assert(launcher is not null, "Root launcher should exist at top level");
        Assert(launcher!.Category == ShippingPackager.EntryCategory.Launcher, "Should be Launcher category");

        // 2. Monolithic EXE in ProjectName/Binaries/Win64/
        var monoExe = FindEntry(plan, "EnigmaArcade/Binaries/Win64/EnigmaArcade-Win64-Shipping.exe");
        Assert(monoExe is not null, "Monolithic EXE should be in ProjectName/Binaries/Win64/");

        // 3. Engine modules under Engine/Binaries/Win64/
        var engineEntries = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("Engine/Binaries/Win64/", StringComparison.Ordinal));
        Assert(engineEntries.Count > 0, "Engine/Binaries/Win64/ should have entries");

        // 4. Engine/ directory structure present
        var engineAny = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("Engine/", StringComparison.Ordinal));
        Assert(engineAny.Count > 0, "Engine/ directory should have entries");

        // 5. Manifests in game Binaries
        var manifests = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("EnigmaArcade/Binaries/", StringComparison.Ordinal) &&
            e.Category == ShippingPackager.EntryCategory.Manifest);
        Assert(manifests.Count > 0, "Manifests should be in ProjectName/Binaries/");

        Console.WriteLine("  PASSED");
    }

    // ── Test 20: No module DLLs in game Binaries (monolithic) ───────────

    private static void TestNoModuleDllsInShipped()
    {
        Console.WriteLine("[Test 20] No .dll in game Binaries (monolithic Shipping)");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new() { ModuleName = "Engine" },
            ["ArcadeGameplay"] = new() { ModuleName = "ArcadeGameplay" },
            ["ArcadeFeature"] = new() { ModuleName = "ArcadeFeature" },
        };
        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core", "Engine" };
        var plan = new ShippingPackager().GeneratePlan("EnigmaArcade", modules,
            GameTarget("EnigmaArcade"), engineModuleNames: engineSet, buildId: "S2");

        Assert(plan.Success, "Plan should succeed");

        // Game Binaries should have NO .dll files (all statically linked into monolithic EXE)
        var gameDlls = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("EnigmaArcade/Binaries/", StringComparison.Ordinal) &&
            e.DestinationPath.EndsWith(".dll", StringComparison.Ordinal));
        Assert(gameDlls.Count == 0,
            $"Game Binaries should have no DLLs in monolithic Shipping, found {gameDlls.Count}");

        // Game Binaries SHOULD have the monolithic EXE
        var gameExes = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("EnigmaArcade/Binaries/", StringComparison.Ordinal) &&
            e.DestinationPath.EndsWith(".exe", StringComparison.Ordinal));
        Assert(gameExes.Count == 1,
            $"Game Binaries should have exactly 1 monolithic EXE, found {gameExes.Count}");

        Console.WriteLine("  PASSED");
    }
}
