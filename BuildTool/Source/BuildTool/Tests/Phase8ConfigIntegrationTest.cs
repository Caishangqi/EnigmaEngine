using System.Text.Json;
using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Phase 8 integration test: validates the full build configuration pipeline
/// across all 5 configurations (Debug/DebugGame/Development/Shipping/Test).
///
/// Verifies:
///   [1-5]   CMake generation succeeds for all 5 configurations
///   [6-10]  DLL naming matches REQ-015 for each configuration
///   [11-15] .modules JSON valid with correct DLL names per configuration
///   [16-20] .target JSON valid with correct structure per configuration
///   [21]    Shipping .modules manifest naming uses platform-config suffix
///   [22]    Development .modules manifest uses short name
///   [23]    Plugin .modules files generated separately
///   [24]    Header-only modules excluded from manifests
///   [25]    Shipping directory structure matches REQ-015
///   [26]    Shipping root launcher present
///   [27]    Engine modules in Engine/Binaries/
///   [28]    Game modules in {GameName}/Binaries/
///   [29]    Plugin modules in Engine/Plugins/
///   [30]    BuildId consistent across all manifest files
/// </summary>
public static class Phase8ConfigIntegrationTest
{
    private static int _passed;
    private static int _failed;

    public static void Run()
    {
        _passed = 0;
        _failed = 0;

        Console.WriteLine("=== Phase 8: Build Configuration Integration ===");
        Console.WriteLine();

        // Build a realistic module graph
        var (modules, resolveResult, targetRules, pluginResult, engineSet) = BuildTestGraph();

        // Section 1: CMake generation for all 5 configs
        TestCMakeAllConfigs(modules, resolveResult);

        // Section 2: DLL naming per config
        TestDllNamingAllConfigs(modules, resolveResult);
        // Section 3: .modules manifest for all 5 configs
        TestModulesManifestAllConfigs(modules, targetRules, pluginResult);

        // Section 4: .target manifest for all 5 configs
        TestTargetManifestAllConfigs(modules, targetRules, pluginResult);

        // Section 5: Manifest naming and plugin separation
        TestManifestNamingAndPlugins(modules, targetRules, pluginResult);

        // Section 6: Shipping directory structure
        TestShippingStructure(modules, targetRules, pluginResult, engineSet);

        // Section 7: BuildId consistency
        TestBuildIdConsistency(modules, targetRules, pluginResult);

        Console.WriteLine();
        Console.WriteLine($"=== {_passed}/{_passed + _failed} tests passed ===");

        if (_failed > 0)
            throw new Exception($"Phase8ConfigIntegrationTest: {_failed} test(s) failed.");
    }

    // ── Test graph setup ────────────────────────────────────

    private static (
        Dictionary<string, ModuleRules> modules,
        DependencyResolver.ResolveResult resolveResult,
        TargetRules targetRules,
        PluginScanner.ScanResult pluginResult,
        HashSet<string> engineSet
    ) BuildTestGraph()
    {
        // Simulate: Engine modules (Core, Engine), Game module (ArcadeGameplay),
        // Plugin (Rewind with module Rewind), Header-only (nlohmann_json)
        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new()
            {
                ModuleName = "Core",
            },
            ["Engine"] = new()
            {
                ModuleName = "Engine",
                PublicDependencyModuleNames = { "Core" },
            },
            ["ArcadeGameplay"] = new()
            {
                ModuleName = "ArcadeGameplay",
                PublicDependencyModuleNames = { "Engine" },
                PrivateDependencyModuleNames = { "nlohmann_json" },
            },
            ["nlohmann_json"] = new()
            {
                ModuleName = "nlohmann_json",
                IsHeaderOnly = true,
            },
            ["Rewind"] = new()
            {
                ModuleName = "Rewind",
                PublicDependencyModuleNames = { "Core" },
            },
        };

        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);

        var targetRules = new TargetRules
        {
            TargetName = "EnigmaArcade",
            Type = TargetType.Game,
            ExtraModuleNames = { "ArcadeGameplay" },
        };

        var pluginResult = new PluginScanner.ScanResult
        {
            Modules = { ["Rewind"] = modules["Rewind"] },
            EnabledPlugins =
            {
                ["RewindPlugin"] = new PluginDescriptor
                {
                    FileVersion = 1,
                    FriendlyName = "Rewind",
                    Modules = { new ModuleDescriptor { Name = "Rewind" } },
                }
            },
        };

        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core", "Engine" };

        return (modules, resolveResult, targetRules, pluginResult, engineSet);
    }
    // ── Section 1: CMake generation for all 5 configs ─────────

    private static void TestCMakeAllConfigs(
        Dictionary<string, ModuleRules> modules,
        DependencyResolver.ResolveResult resolveResult)
    {
        Console.WriteLine("--- CMake Generation (all 5 configs) ---");

        var configs = Enum.GetValues<BuildConfiguration>();
        int testNum = 1;
        foreach (var config in configs)
        {
            var gen = new CMakeGenerator();
            var result = gen.Generate("EnigmaArcade", modules, resolveResult,
                "C:/Projects/EnigmaArcade", configuration: config);

            Check(result.Success,
                $"[{testNum}]  CMake generation succeeds for {config}");

            if (result.Success)
            {
                // Verify config-specific macro present
                var expectedMacro = $"ENIGMA_BUILD_{config.ToString().ToUpperInvariant()}=1";
                Check(result.Content.Contains(expectedMacro),
                    $"     CMake contains {expectedMacro}");
            }
            testNum++;
        }
    }

    // ── Section 2: DLL naming per config ────────────────────

    private static void TestDllNamingAllConfigs(
        Dictionary<string, ModuleRules> modules,
        DependencyResolver.ResolveResult resolveResult)
    {
        Console.WriteLine("\n--- DLL Naming (REQ-015) ---");

        var expectations = new (BuildConfiguration config, string expectedPattern)[]
        {
            (BuildConfiguration.Debug,       "EnigmaArcade-Core-Win64-Debug"),
            (BuildConfiguration.DebugGame,   "EnigmaArcade-Core-Win64-DebugGame"),
            (BuildConfiguration.Development, "EnigmaArcade-Core\""),  // short name, no platform
            (BuildConfiguration.Shipping,    "EnigmaArcade-Core-Win64-Shipping"),
            (BuildConfiguration.Test,        "EnigmaArcade-Core-Win64-Test"),
        };

        int testNum = 6;
        foreach (var (config, pattern) in expectations)
        {
            var gen = new CMakeGenerator();
            var result = gen.Generate("EnigmaArcade", modules, resolveResult,
                "C:/Projects/EnigmaArcade", configuration: config);

            Check(result.Success && result.Content.Contains(pattern),
                $"[{testNum}]  {config} DLL naming: {pattern.TrimEnd('"')}");
            testNum++;
        }
    }

    // ── Section 3: .modules manifest for all 5 configs ──────

    private static void TestModulesManifestAllConfigs(
        Dictionary<string, ModuleRules> modules,
        TargetRules targetRules,
        PluginScanner.ScanResult pluginResult)
    {
        Console.WriteLine("\n--- .modules Manifest (all 5 configs) ---");

        var configs = Enum.GetValues<BuildConfiguration>();
        int testNum = 11;
        foreach (var config in configs)
        {
            var gen = new ManifestGenerator();
            var result = gen.Generate("EnigmaArcade", modules, config,
                targetRules: targetRules, pluginScanResult: pluginResult, buildId: "PHASE8TEST");

            Check(result.Success, $"[{testNum}]  .modules generated for {config}");

            if (result.Success)
            {
                // Find the project .modules file
                var baseName = ManifestGenerator.GetManifestBaseName("EnigmaArcade", config, "Win64");
                var key = $"Binaries/Win64/{baseName}.modules";
                Check(result.Files.ContainsKey(key), $"     .modules file at {key}");

                if (result.Files.TryGetValue(key, out var json))
                {
                    var doc = JsonDocument.Parse(json);
                    var root = doc.RootElement;
                    Check(root.TryGetProperty("BuildId", out _) && root.TryGetProperty("Modules", out _),
                        $"     .modules JSON has BuildId and Modules");

                    // Verify DLL name format in modules
                    var expectedDll = ManifestGenerator.GetDllFileName("EnigmaArcade", "Core", config, "Win64");
                    var coreDll = root.GetProperty("Modules").GetProperty("Core").GetString();
                    Check(coreDll == expectedDll,
                        $"     Core DLL = {coreDll}");
                }
            }
            testNum++;
        }
    }
    // ── Section 4: .target manifest for all 5 configs ─────────

    private static void TestTargetManifestAllConfigs(
        Dictionary<string, ModuleRules> modules,
        TargetRules targetRules,
        PluginScanner.ScanResult pluginResult)
    {
        Console.WriteLine("\n--- .target Manifest (all 5 configs) ---");

        var configs = Enum.GetValues<BuildConfiguration>();
        int testNum = 16;
        foreach (var config in configs)
        {
            var gen = new ManifestGenerator();
            var result = gen.Generate("EnigmaArcade", modules, config,
                targetRules: targetRules, pluginScanResult: pluginResult, buildId: "PHASE8TEST");

            var baseName = ManifestGenerator.GetManifestBaseName("EnigmaArcade", config, "Win64");
            var key = $"Binaries/Win64/{baseName}.target";

            if (result.Success && result.Files.TryGetValue(key, out var json))
            {
                var root = JsonDocument.Parse(json).RootElement;

                var hasTarget = root.GetProperty("TargetName").GetString() == "EnigmaArcade";
                var hasPlatform = root.GetProperty("Platform").GetString() == "Win64";
                var hasConfig = root.GetProperty("Configuration").GetString() == config.ToString();
                var hasType = root.GetProperty("TargetType").GetString() == "Game";
                var hasVersion = root.TryGetProperty("Version", out var ver)
                    && ver.GetProperty("BuildId").GetString() == "PHASE8TEST";
                var hasProducts = root.GetProperty("BuildProducts").GetArrayLength() > 0;

                Check(hasTarget && hasPlatform && hasConfig && hasType && hasVersion && hasProducts,
                    $"[{testNum}]  .target valid for {config} (TargetName, Platform, Config, Version, BuildProducts)");
            }
            else
            {
                Check(false, $"[{testNum}]  .target generated for {config}");
            }
            testNum++;
        }
    }

    // ── Section 5: Manifest naming and plugin separation ────

    private static void TestManifestNamingAndPlugins(
        Dictionary<string, ModuleRules> modules,
        TargetRules targetRules,
        PluginScanner.ScanResult pluginResult)
    {
        Console.WriteLine("\n--- Manifest Naming & Plugin Separation ---");

        // [21] Shipping uses platform-config suffix
        {
            var gen = new ManifestGenerator();
            var result = gen.Generate("EnigmaArcade", modules, BuildConfiguration.Shipping,
                targetRules: targetRules, pluginScanResult: pluginResult, buildId: "N1");

            Check(result.Files.ContainsKey("Binaries/Win64/EnigmaArcade-Win64-Shipping.modules"),
                "[21] Shipping .modules uses -Win64-Shipping suffix");
        }

        // [22] Development uses short name
        {
            var gen = new ManifestGenerator();
            var result = gen.Generate("EnigmaArcade", modules, BuildConfiguration.Development,
                targetRules: targetRules, pluginScanResult: pluginResult, buildId: "N2");

            Check(result.Files.ContainsKey("Binaries/Win64/EnigmaArcade.modules"),
                "[22] Development .modules uses short name");
        }

        // [23] Plugin .modules generated separately
        {
            var gen = new ManifestGenerator();
            var result = gen.Generate("EnigmaArcade", modules, BuildConfiguration.Development,
                pluginScanResult: pluginResult, buildId: "N3");

            var pluginKey = "Plugins/RewindPlugin/Binaries/Win64/EnigmaArcade.modules";
            Check(result.Files.ContainsKey(pluginKey),
                "[23] Plugin .modules file generated at Plugins/RewindPlugin/...");

            if (result.Files.TryGetValue(pluginKey, out var pJson))
            {
                var pRoot = JsonDocument.Parse(pJson).RootElement;
                var hasMod = pRoot.GetProperty("Modules").TryGetProperty("Rewind", out _);
                Check(hasMod, "     Plugin .modules contains Rewind module");
            }
        }

        // [24] Header-only excluded from manifests
        {
            var gen = new ManifestGenerator();
            var result = gen.Generate("EnigmaArcade", modules, BuildConfiguration.Development,
                targetRules: targetRules, buildId: "N4");

            var json = result.Files["Binaries/Win64/EnigmaArcade.modules"];
            Check(!json.Contains("nlohmann_json"),
                "[24] Header-only (nlohmann_json) excluded from .modules");
        }
    }
    // ── Section 6: Shipping directory structure ───────────────

    private static void TestShippingStructure(
        Dictionary<string, ModuleRules> modules,
        TargetRules targetRules,
        PluginScanner.ScanResult pluginResult,
        HashSet<string> engineSet)
    {
        Console.WriteLine("\n--- Shipping Directory Structure (REQ-015) ---");

        var packager = new ShippingPackager();
        var plan = packager.GeneratePlan("EnigmaArcade", modules, targetRules,
            pluginScanResult: pluginResult, engineModuleNames: engineSet, buildId: "SHIP01");

        Check(plan.Success,
            "[25] Shipping package plan generated successfully");

        // [26] Root launcher
        var launcher = plan.Entries.Find(e => e.DestinationPath == "EnigmaArcade.exe");
        Check(launcher is not null && launcher.Category == ShippingPackager.EntryCategory.Launcher,
            "[26] Root launcher: EnigmaArcade.exe at root");

        // [27] Engine modules in Engine/Binaries/
        var coreInEngine = plan.Entries.Find(e =>
            e.DestinationPath == "Engine/Binaries/Win64/EnigmaArcade-Core-Win64-Shipping.dll");
        var engineInEngine = plan.Entries.Find(e =>
            e.DestinationPath == "Engine/Binaries/Win64/EnigmaArcade-Engine-Win64-Shipping.dll");
        Check(coreInEngine is not null && engineInEngine is not null,
            "[27] Engine modules (Core, Engine) in Engine/Binaries/Win64/");

        // [28] Game exe in {GameName}/Binaries/ (no module DLLs — monolithic)
        var gameExe = plan.Entries.Find(e =>
            e.DestinationPath == "EnigmaArcade/Binaries/Win64/EnigmaArcade-Win64-Shipping.exe");
        var gameDlls = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("EnigmaArcade/Binaries/", StringComparison.Ordinal) &&
            e.DestinationPath.EndsWith(".dll", StringComparison.Ordinal));
        Check(gameExe is not null && gameDlls.Count == 0,
            "[28] Monolithic EXE in EnigmaArcade/Binaries/Win64/, no module DLLs");

        // [29] Plugin modules in Engine/Plugins/
        var pluginDll = plan.Entries.Find(e =>
            e.DestinationPath == "Engine/Plugins/RewindPlugin/Binaries/Win64/EnigmaArcade-Rewind-Win64-Shipping.dll");
        Check(pluginDll is not null,
            "[29] Plugin module (Rewind) in Engine/Plugins/RewindPlugin/Binaries/");

        // Print structure summary
        Console.WriteLine("     Structure summary:");
        var dirs = plan.Entries
            .Select(e => string.Join("/", e.DestinationPath.Split('/').SkipLast(1)))
            .Where(d => d.Length > 0)
            .Distinct()
            .OrderBy(d => d);
        foreach (var d in dirs)
            Console.WriteLine($"       {d}/");
    }

    // ── Section 7: BuildId consistency ──────────────────────

    private static void TestBuildIdConsistency(
        Dictionary<string, ModuleRules> modules,
        TargetRules targetRules,
        PluginScanner.ScanResult pluginResult)
    {
        Console.WriteLine("\n--- BuildId Consistency ---");

        var gen = new ManifestGenerator();
        var result = gen.Generate("EnigmaArcade", modules, BuildConfiguration.Development,
            targetRules: targetRules, pluginScanResult: pluginResult, buildId: "CONSIST01");

        var buildIds = new List<string>();
        foreach (var (_, content) in result.Files)
        {
            var root = JsonDocument.Parse(content).RootElement;
            // .modules has top-level BuildId
            if (root.TryGetProperty("BuildId", out var bid))
                buildIds.Add(bid.GetString()!);
            // .target has Version.BuildId
            if (root.TryGetProperty("Version", out var ver)
                && ver.TryGetProperty("BuildId", out var vbid))
                buildIds.Add(vbid.GetString()!);
        }

        Check(buildIds.Count >= 3 && buildIds.All(id => id == "CONSIST01"),
            $"[30] BuildId consistent across all {buildIds.Count} manifest files");
    }

    // ── Helpers ──────────────────────────────────────────────

    private static void Check(bool cond, string name)
    {
        if (cond) { Console.WriteLine($"  [PASS] {name}"); ++_passed; }
        else      { Console.WriteLine($"  [FAIL] {name}"); ++_failed; }
    }
}
