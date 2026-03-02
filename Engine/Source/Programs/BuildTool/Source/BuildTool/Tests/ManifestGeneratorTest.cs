using System.Text.Json;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Tests for ManifestGenerator: .modules and .target JSON manifest generation.
/// </summary>
public static class ManifestGeneratorTest
{
    public static void Run()
    {
        Console.WriteLine("=== ManifestGenerator Tests ===");
        Console.WriteLine();

        TestModulesJsonFormat();
        TestModulesDevDllNaming();
        TestModulesDebugGameDllNaming();
        TestTargetJsonFormat();
        TestTargetBuildProducts();
        TestPluginSeparateModulesFile();
        TestPluginModulesExcludedFromProject();
        TestHeaderOnlyModulesExcluded();
        TestManifestFileNamingDevelopment();
        TestManifestFileNamingNonDevelopment();
        TestBuildIdTimestamp();
        TestExplicitBuildId();
        TestFailsOnEmptyModules();
        TestAllFiveConfigurations();
        TestLaunchExeNaming();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    // ── Helpers ──────────────────────────────────────────────

    private static void Assert(bool condition, string message)
    {
        if (!condition)
            throw new Exception($"Assertion failed: {message}");
    }
    private static Dictionary<string, ModuleRules> MakeModules(params string[] names)
    {
        var dict = new Dictionary<string, ModuleRules>(StringComparer.Ordinal);
        foreach (var n in names)
            dict[n] = new ModuleRules { ModuleName = n };
        return dict;
    }

    private static TargetRules MakeTarget(string name = "TestGame") =>
        new() { TargetName = name, Type = TargetType.Game, ExtraModuleNames = { name } };

    private static JsonElement ParseJson(string json) =>
        JsonDocument.Parse(json).RootElement;

    // ── Test 1: .modules JSON structure ──────────────────────

    /// <summary>
    /// .modules JSON must contain BuildId (string) and Modules (object with module→DLL mapping).
    /// </summary>
    private static void TestModulesJsonFormat()
    {
        Console.WriteLine("[Test 1] .modules JSON format (BuildId + Modules object)");

        var modules = MakeModules("Core", "Engine");
        var result = new ManifestGenerator().Generate("MyGame", modules, buildId: "TEST001");

        Assert(result.Success, "Generation should succeed");
        var modulesFile = result.Files["Binaries/Win64/MyGame.modules"];
        var root = ParseJson(modulesFile);

        Assert(root.GetProperty("BuildId").GetString() == "TEST001",
            "BuildId should be TEST001");
        var mods = root.GetProperty("Modules");
        Assert(mods.GetProperty("Core").GetString() == "MyGame-Core.dll",
            "Core DLL name");
        Assert(mods.GetProperty("Engine").GetString() == "MyGame-Engine.dll",
            "Engine DLL name");
    }

    // ── Test 2: Development DLL naming ──────────────────────

    private static void TestModulesDevDllNaming()
    {
        Console.WriteLine("[Test 2] Development DLL naming: {Project}-{Module}.dll");

        var modules = MakeModules("ArcadeCore");
        var result = new ManifestGenerator().Generate("EnigmaArcade", modules,
            BuildConfiguration.Development, buildId: "B1");

        var json = ParseJson(result.Files["Binaries/Win64/EnigmaArcade.modules"]);
        var dll = json.GetProperty("Modules").GetProperty("ArcadeCore").GetString();
        Assert(dll == "EnigmaArcade-ArcadeCore.dll",
            $"Expected EnigmaArcade-ArcadeCore.dll, got {dll}");
    }

    // ── Test 3: DebugGame DLL naming ────────────────────────

    private static void TestModulesDebugGameDllNaming()
    {
        Console.WriteLine("[Test 3] DebugGame DLL naming: {Project}-{Module}-Win64-DebugGame.dll");

        var modules = MakeModules("ArcadeCore");
        var result = new ManifestGenerator().Generate("EnigmaArcade", modules,
            BuildConfiguration.DebugGame, buildId: "B2");

        var key = "Binaries/Win64/EnigmaArcade-Win64-DebugGame.modules";
        Assert(result.Files.ContainsKey(key), $"Should have file at {key}");
        var json = ParseJson(result.Files[key]);
        var dll = json.GetProperty("Modules").GetProperty("ArcadeCore").GetString();
        Assert(dll == "EnigmaArcade-ArcadeCore-Win64-DebugGame.dll",
            $"Expected DebugGame DLL name, got {dll}");
    }
    // ── Test 4: .target JSON structure ────────────────────────

    private static void TestTargetJsonFormat()
    {
        Console.WriteLine("[Test 4] .target JSON format (TargetName, Platform, Config, Version, BuildProducts)");

        var modules = MakeModules("Core");
        var target = MakeTarget("EnigmaArcade");
        var result = new ManifestGenerator().Generate("EnigmaArcade", modules,
            targetRules: target, buildId: "T001");

        var json = ParseJson(result.Files["Binaries/Win64/EnigmaArcade.target"]);

        Assert(json.GetProperty("TargetName").GetString() == "EnigmaArcade", "TargetName");
        Assert(json.GetProperty("Platform").GetString() == "Win64", "Platform");
        Assert(json.GetProperty("Configuration").GetString() == "Development", "Configuration");
        Assert(json.GetProperty("TargetType").GetString() == "Game", "TargetType");
        Assert(json.GetProperty("Project").GetString() == "../../EnigmaArcade.eproject", "Project");
        Assert(json.GetProperty("Launch").GetString() == "EnigmaArcade.exe", "Launch");

        var ver = json.GetProperty("Version");
        Assert(ver.GetProperty("MajorVersion").GetInt32() == 1, "MajorVersion");
        Assert(ver.GetProperty("MinorVersion").GetInt32() == 0, "MinorVersion");
        Assert(ver.GetProperty("PatchVersion").GetInt32() == 0, "PatchVersion");
        Assert(ver.GetProperty("BuildId").GetString() == "T001", "Version.BuildId");
    }

    // ── Test 5: .target BuildProducts ───────────────────────

    private static void TestTargetBuildProducts()
    {
        Console.WriteLine("[Test 5] .target BuildProducts lists all module DLLs");

        var modules = MakeModules("Core", "Engine");
        var target = MakeTarget("MyGame");
        var result = new ManifestGenerator().Generate("MyGame", modules,
            targetRules: target, buildId: "T002");

        var json = ParseJson(result.Files["Binaries/Win64/MyGame.target"]);
        var products = json.GetProperty("BuildProducts");
        Assert(products.GetArrayLength() == 2, "Should have 2 build products");

        // Sorted alphabetically by Path
        var p0 = products[0];
        Assert(p0.GetProperty("Path").GetString() == "MyGame-Core.dll", "First product path");
        Assert(p0.GetProperty("Type").GetString() == "DynamicLibrary", "First product type");
        var p1 = products[1];
        Assert(p1.GetProperty("Path").GetString() == "MyGame-Engine.dll", "Second product path");
    }

    // ── Test 6: Plugin separate .modules file ───────────────

    private static void TestPluginSeparateModulesFile()
    {
        Console.WriteLine("[Test 6] Plugin gets separate .modules file in Plugins/{Name}/Binaries/");

        var modules = MakeModules("Core");
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

        var result = new ManifestGenerator().Generate("MyGame", modules,
            pluginScanResult: pluginResult, buildId: "P001");

        var pluginKey = "Plugins/RewindPlugin/Binaries/Win64/MyGame.modules";
        Assert(result.Files.ContainsKey(pluginKey), $"Should have plugin .modules at {pluginKey}");

        var json = ParseJson(result.Files[pluginKey]);
        Assert(json.GetProperty("BuildId").GetString() == "P001", "Plugin BuildId matches");
        var mods = json.GetProperty("Modules");
        Assert(mods.GetProperty("Rewind").GetString() == "MyGame-Rewind.dll", "Plugin module DLL");
    }
    // ── Test 7: Plugin modules excluded from project .modules ─

    private static void TestPluginModulesExcludedFromProject()
    {
        Console.WriteLine("[Test 7] Plugin modules excluded from project .modules file");

        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Rewind"] = new() { ModuleName = "Rewind" }, // also in plugin scan
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

        var result = new ManifestGenerator().Generate("MyGame", modules,
            pluginScanResult: pluginResult, buildId: "P002");

        var projectJson = ParseJson(result.Files["Binaries/Win64/MyGame.modules"]);
        var mods = projectJson.GetProperty("Modules");

        Assert(mods.TryGetProperty("Core", out _), "Core should be in project .modules");
        Assert(!mods.TryGetProperty("Rewind", out _), "Rewind should NOT be in project .modules");
    }

    // ── Test 8: Header-only modules excluded ────────────────

    private static void TestHeaderOnlyModulesExcluded()
    {
        Console.WriteLine("[Test 8] Header-only modules excluded from .modules and BuildProducts");

        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["nlohmann_json"] = new() { ModuleName = "nlohmann_json", IsHeaderOnly = true },
        };
        var target = MakeTarget("MyGame");
        var result = new ManifestGenerator().Generate("MyGame", modules,
            targetRules: target, buildId: "H001");

        // .modules should not contain header-only
        var modulesJson = ParseJson(result.Files["Binaries/Win64/MyGame.modules"]);
        var mods = modulesJson.GetProperty("Modules");
        Assert(mods.TryGetProperty("Core", out _), "Core present");
        Assert(!mods.TryGetProperty("nlohmann_json", out _), "nlohmann_json excluded");

        // .target BuildProducts should not contain header-only
        var targetJson = ParseJson(result.Files["Binaries/Win64/MyGame.target"]);
        var products = targetJson.GetProperty("BuildProducts");
        Assert(products.GetArrayLength() == 1, "Only 1 build product (Core)");
    }

    // ── Test 9: Manifest file naming (Development) ──────────

    private static void TestManifestFileNamingDevelopment()
    {
        Console.WriteLine("[Test 9] Development manifest: {Project}.modules / .target");

        var modules = MakeModules("Core");
        var target = MakeTarget("MyGame");
        var result = new ManifestGenerator().Generate("MyGame", modules,
            BuildConfiguration.Development, targetRules: target, buildId: "D1");

        Assert(result.Files.ContainsKey("Binaries/Win64/MyGame.modules"), ".modules path");
        Assert(result.Files.ContainsKey("Binaries/Win64/MyGame.target"), ".target path");
    }

    // ── Test 10: Manifest file naming (non-Development) ─────

    private static void TestManifestFileNamingNonDevelopment()
    {
        Console.WriteLine("[Test 10] Non-Development manifest: {Project}-{Platform}-{Config}.modules");

        var modules = MakeModules("Core");
        var target = MakeTarget("MyGame");
        var result = new ManifestGenerator().Generate("MyGame", modules,
            BuildConfiguration.Shipping, targetRules: target, buildId: "S1");

        Assert(result.Files.ContainsKey("Binaries/Win64/MyGame-Win64-Shipping.modules"),
            "Shipping .modules path");
        Assert(result.Files.ContainsKey("Binaries/Win64/MyGame-Win64-Shipping.target"),
            "Shipping .target path");
    }
    // ── Test 11: BuildId from timestamp ────────────────────────

    private static void TestBuildIdTimestamp()
    {
        Console.WriteLine("[Test 11] Auto-generated BuildId is 14-digit timestamp");

        var id = ManifestGenerator.GenerateBuildId();
        Assert(id.Length == 14, $"BuildId length should be 14, got {id.Length}");
        Assert(long.TryParse(id, out _), "BuildId should be numeric");
    }

    // ── Test 12: Explicit BuildId ───────────────────────────

    private static void TestExplicitBuildId()
    {
        Console.WriteLine("[Test 12] Explicit BuildId propagates to all files");

        var modules = MakeModules("Core");
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
        var target = MakeTarget("MyGame");
        var result = new ManifestGenerator().Generate("MyGame", modules,
            targetRules: target, pluginScanResult: pluginResult, buildId: "CUSTOM42");

        // Check project .modules
        var projMod = ParseJson(result.Files["Binaries/Win64/MyGame.modules"]);
        Assert(projMod.GetProperty("BuildId").GetString() == "CUSTOM42", "Project .modules BuildId");

        // Check .target Version.BuildId
        var tgt = ParseJson(result.Files["Binaries/Win64/MyGame.target"]);
        Assert(tgt.GetProperty("Version").GetProperty("BuildId").GetString() == "CUSTOM42",
            ".target Version.BuildId");

        // Check plugin .modules
        var plugMod = ParseJson(result.Files["Plugins/FeatPlugin/Binaries/Win64/MyGame.modules"]);
        Assert(plugMod.GetProperty("BuildId").GetString() == "CUSTOM42", "Plugin .modules BuildId");
    }

    // ── Test 13: Fails on empty modules ─────────────────────

    private static void TestFailsOnEmptyModules()
    {
        Console.WriteLine("[Test 13] Fails when no modules provided");

        var empty = new Dictionary<string, ModuleRules>();
        var result = new ManifestGenerator().Generate("MyGame", empty, buildId: "X");

        Assert(!result.Success, "Should fail");
        Assert(result.Error!.Contains("no modules"), $"Error should mention 'no modules': {result.Error}");
    }

    // ── Test 14: All 5 configurations ───────────────────────

    private static void TestAllFiveConfigurations()
    {
        Console.WriteLine("[Test 14] All 5 configurations produce correct DLL and manifest names");

        var configs = new[]
        {
            (BuildConfiguration.Debug,       "MyGame-Core-Win64-Debug.dll",       "MyGame-Win64-Debug"),
            (BuildConfiguration.DebugGame,   "MyGame-Core-Win64-DebugGame.dll",   "MyGame-Win64-DebugGame"),
            (BuildConfiguration.Development, "MyGame-Core.dll",                   "MyGame"),
            (BuildConfiguration.Shipping,    "MyGame-Core-Win64-Shipping.dll",    "MyGame-Win64-Shipping"),
            (BuildConfiguration.Test,        "MyGame-Core-Win64-Test.dll",        "MyGame-Win64-Test"),
        };

        foreach (var (config, expectedDll, expectedBase) in configs)
        {
            var modules = MakeModules("Core");
            var result = new ManifestGenerator().Generate("MyGame", modules, config, buildId: "C1");

            var modulesKey = $"Binaries/Win64/{expectedBase}.modules";
            Assert(result.Files.ContainsKey(modulesKey),
                $"{config}: expected .modules at {modulesKey}");

            var json = ParseJson(result.Files[modulesKey]);
            var dll = json.GetProperty("Modules").GetProperty("Core").GetString();
            Assert(dll == expectedDll,
                $"{config}: expected DLL {expectedDll}, got {dll}");
        }
    }

    // ── Test 15: Launch exe naming ──────────────────────────

    private static void TestLaunchExeNaming()
    {
        Console.WriteLine("[Test 15] Launch exe: Development short, others with platform-config");

        var modules = MakeModules("Core");
        var target = MakeTarget("MyGame");

        // Development
        var devResult = new ManifestGenerator().Generate("MyGame", modules,
            BuildConfiguration.Development, targetRules: target, buildId: "L1");
        var devJson = ParseJson(devResult.Files["Binaries/Win64/MyGame.target"]);
        Assert(devJson.GetProperty("Launch").GetString() == "MyGame.exe",
            "Development launch exe");

        // Shipping
        var shipResult = new ManifestGenerator().Generate("MyGame", modules,
            BuildConfiguration.Shipping, targetRules: target, buildId: "L2");
        var shipJson = ParseJson(shipResult.Files["Binaries/Win64/MyGame-Win64-Shipping.target"]);
        Assert(shipJson.GetProperty("Launch").GetString() == "MyGame-Win64-Shipping.exe",
            "Shipping launch exe");
    }
}
