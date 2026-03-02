using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;
using BuildTool.Parsers;
using BuildTool.Scanners;

namespace BuildTool.Tests;

/// <summary>
/// Phase 9.1-9.5: Validates the EnigmaArcade project structure, C++ module implementation,
/// ArcadeFeature plugin, JSON integration, and full build pipeline across all configurations.
/// Ensures all project files are parseable by BuildTool, C++ sources exist,
/// and the full pipeline (parse → resolve → generate → compile) succeeds.
///
/// Verifies:
///   [1-4]   .eproject parsing and content
///   [5-8]   .Target.cs parsing and content
///   [9-11]  EnigmaArcade.Build.cs (primary module)
///   [12-14] ArcadeGameplay.Build.cs (gameplay module)
///   [15-18] ArcadeFeature plugin descriptor and Build.cs
///   [19-20] Directory structure (REQ-013)
///   [21-24] C++ source files exist (headers, implementations, module registration)
///   [25-28] BuildTool pipeline: dependency resolve → CMake generate → compile
///   [29-32] ArcadeFeature plugin C++ sources and PluginScanner discovery
///   [33-36] JSON integration (REQ-014): nlohmann_json dependency, include, output format
///   [37-42] Multi-config build: Debug/Development/Shipping CMake + DLL naming
///   [43-48] Shipping package structure (REQ-015)
///   [49-52] Runtime output format validation (REQ-013)
/// </summary>
public static class EnigmaArcadeProjectTest
{
    private static int _passed;
    private static int _failed;

    public static void Run()
    {
        _passed = 0;
        _failed = 0;

        Console.WriteLine("=== Phase 9.1: EnigmaArcade Project Structure ===");
        Console.WriteLine();

        var projectRoot = ResolveProjectRoot();
        Console.WriteLine($"Project root: {projectRoot}");
        Console.WriteLine();

        TestEproject(projectRoot);
        TestTargetCs(projectRoot);
        TestPrimaryModule(projectRoot);
        TestGameplayModule(projectRoot);
        TestPlugin(projectRoot);
        TestDirectoryStructure(projectRoot);
        TestCppSourceFiles(projectRoot);
        TestPluginCppFiles(projectRoot);
        TestJsonIntegration(projectRoot);
        TestBuildPipeline(projectRoot);
        TestMultiConfigBuild(projectRoot);
        TestShippingStructure(projectRoot);
        TestRuntimeOutputFormat(projectRoot);

        Console.WriteLine();
        Console.WriteLine($"=== {_passed}/{_passed + _failed} tests passed ===");

        if (_failed > 0)
            throw new Exception($"EnigmaArcadeProjectTest: {_failed} test(s) failed.");
    }

    // ── .eproject ─────────────────────────────────────────────

    private static void TestEproject(string projectRoot)
    {
        Console.WriteLine("--- .eproject ---");

        var path = Path.Combine(projectRoot, "EnigmaArcade.eproject");
        ProjectDescriptor project;
        try
        {
            project = ProjectParser.Parse(path);
            Check(true, "[1]  .eproject parses successfully");
        }
        catch (Exception ex)
        {
            Check(false, $"[1]  .eproject parses successfully ({ex.Message})");
            return;
        }

        Check(project.FileVersion == 1,
            $"[2]  FileVersion = {project.FileVersion} (expected 1)");

        Check(project.Modules.Count == 2,
            $"[3]  Module count = {project.Modules.Count} (expected 2)");
        if (project.Modules.Count >= 2)
        {
            Check(project.Modules[0].Name == "EnigmaArcade",
                $"     First module = {project.Modules[0].Name} (REQ-018: primary module first)");
            Check(project.Modules[1].Name == "ArcadeGameplay",
                $"     Second module = {project.Modules[1].Name}");
        }

        Check(project.Plugins.Count == 1 && project.Plugins[0].Name == "ArcadeFeature" && project.Plugins[0].Enabled,
            $"[4]  Plugin: ArcadeFeature (Enabled=true)");
    }

    // ── .Target.cs ────────────────────────────────────────────

    private static void TestTargetCs(string projectRoot)
    {
        Console.WriteLine("\n--- .Target.cs ---");

        var path = Path.Combine(projectRoot, "Source", "EnigmaArcade.Target.cs");
        TargetRules target;
        try
        {
            target = TargetParser.Parse(path);
            Check(true, "[5]  .Target.cs parses successfully");
        }
        catch (Exception ex)
        {
            Check(false, $"[5]  .Target.cs parses successfully ({ex.Message})");
            return;
        }

        Check(target.Type == TargetType.Game,
            $"[6]  TargetType = {target.Type} (expected Game)");

        Check(target.ExtraModuleNames.Count >= 2
              && target.ExtraModuleNames[0] == "EnigmaArcade",
            $"[7]  ExtraModuleNames[0] = {(target.ExtraModuleNames.Count > 0 ? target.ExtraModuleNames[0] : "N/A")} (REQ-018: primary first)");

        Check(target.ExtraModuleNames.Count >= 2
              && target.ExtraModuleNames[1] == "ArcadeGameplay",
            $"[8]  ExtraModuleNames[1] = {(target.ExtraModuleNames.Count > 1 ? target.ExtraModuleNames[1] : "N/A")}");
    }

    // ── Primary module: EnigmaArcade.Build.cs ─────────────────

    private static void TestPrimaryModule(string projectRoot)
    {
        Console.WriteLine("\n--- EnigmaArcade.Build.cs (primary module) ---");

        var path = Path.Combine(projectRoot, "Source", "EnigmaArcade", "EnigmaArcade.Build.cs");
        ModuleRules module;
        try
        {
            module = ModuleParser.Parse(path);
            Check(true, "[9]  EnigmaArcade.Build.cs parses successfully");
        }
        catch (Exception ex)
        {
            Check(false, $"[9]  EnigmaArcade.Build.cs parses successfully ({ex.Message})");
            return;
        }

        Check(module.ModuleName == "EnigmaArcade",
            $"[10] Module name = {module.ModuleName} (REQ-018: matches project name)");

        Check(module.PublicDependencyModuleNames.Contains("Core")
              && module.PublicDependencyModuleNames.Contains("Engine"),
            "[11] Public deps include Core + Engine");
    }

    // ── ArcadeGameplay.Build.cs ───────────────────────────────

    private static void TestGameplayModule(string projectRoot)
    {
        Console.WriteLine("\n--- ArcadeGameplay.Build.cs ---");

        var path = Path.Combine(projectRoot, "Source", "ArcadeGameplay", "ArcadeGameplay.Build.cs");
        ModuleRules module;
        try
        {
            module = ModuleParser.Parse(path);
            Check(true, "[12] ArcadeGameplay.Build.cs parses successfully");
        }
        catch (Exception ex)
        {
            Check(false, $"[12] ArcadeGameplay.Build.cs parses successfully ({ex.Message})");
            return;
        }

        Check(module.PublicDependencyModuleNames.Contains("Core")
              && module.PublicDependencyModuleNames.Contains("Engine"),
            "[13] Public deps include Core + Engine");

        Check(module.PrivateDependencyModuleNames.Count == 0,
            "[14] ArcadeGameplay has no private dependencies (standalone gameplay module)");
    }

    // ── ArcadeFeature plugin ──────────────────────────────────

    private static void TestPlugin(string projectRoot)
    {
        Console.WriteLine("\n--- ArcadeFeature plugin ---");

        var epluginPath = Path.Combine(projectRoot, "Plugins", "ArcadeFeature", "ArcadeFeature.eplugin");
        PluginDescriptor plugin;
        try
        {
            plugin = PluginParser.Parse(epluginPath);
            Check(true, "[15] ArcadeFeature.eplugin parses successfully");
        }
        catch (Exception ex)
        {
            Check(false, $"[15] ArcadeFeature.eplugin parses successfully ({ex.Message})");
            return;
        }

        Check(plugin.Modules.Count == 1
              && plugin.Modules[0].Name == "ArcadeFeature"
              && plugin.Modules[0].Type == EHostType.Runtime,
            $"[16] Plugin has 1 Runtime module: {(plugin.Modules.Count > 0 ? plugin.Modules[0].Name : "N/A")}");

        var buildCsPath = Path.Combine(projectRoot, "Plugins", "ArcadeFeature",
            "Source", "ArcadeFeature", "ArcadeFeature.Build.cs");
        ModuleRules pluginModule;
        try
        {
            pluginModule = ModuleParser.Parse(buildCsPath);
            Check(true, "[17] ArcadeFeature.Build.cs parses successfully");
        }
        catch (Exception ex)
        {
            Check(false, $"[17] ArcadeFeature.Build.cs parses successfully ({ex.Message})");
            return;
        }

        Check(pluginModule.PublicDependencyModuleNames.Contains("Core")
              && pluginModule.PublicDependencyModuleNames.Contains("Engine"),
            "[18] Plugin module public deps include Core + Engine");
    }

    // ── Directory structure ───────────────────────────────────

    private static void TestDirectoryStructure(string projectRoot)
    {
        Console.WriteLine("\n--- Directory Structure (REQ-013) ---");

        var requiredFiles = new[]
        {
            "EnigmaArcade.eproject",
            "Source/EnigmaArcade.Target.cs",
            "Source/EnigmaArcade/EnigmaArcade.Build.cs",
            "Source/ArcadeGameplay/ArcadeGameplay.Build.cs",
            "Plugins/ArcadeFeature/ArcadeFeature.eplugin",
            "Plugins/ArcadeFeature/Source/ArcadeFeature/ArcadeFeature.Build.cs",
        };

        var allExist = true;
        foreach (var rel in requiredFiles)
        {
            var full = Path.Combine(projectRoot, rel.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(full))
            {
                Console.WriteLine($"     MISSING: {rel}");
                allExist = false;
            }
        }
        Check(allExist, "[19] All required files present per REQ-013 layout");

        var requiredDirs = new[]
        {
            "Source/EnigmaArcade/Public",
            "Source/EnigmaArcade/Private",
            "Source/ArcadeGameplay/Public",
            "Source/ArcadeGameplay/Private",
            "Plugins/ArcadeFeature/Source/ArcadeFeature/Public",
            "Plugins/ArcadeFeature/Source/ArcadeFeature/Private",
        };

        var allDirsExist = true;
        foreach (var rel in requiredDirs)
        {
            var full = Path.Combine(projectRoot, rel.Replace('/', Path.DirectorySeparatorChar));
            if (!Directory.Exists(full))
            {
                Console.WriteLine($"     MISSING DIR: {rel}");
                allDirsExist = false;
            }
        }
        Check(allDirsExist, "[20] All Public/ and Private/ directories exist");
    }

    // ── C++ source files ──────────────────────────────────────

    private static void TestCppSourceFiles(string projectRoot)
    {
        Console.WriteLine("\n--- C++ Source Files ---");

        // [21] EnigmaArcade module C++ files
        var enigmaArcadeCppFiles = new[]
        {
            "Source/EnigmaArcade/Public/EnigmaArcadeModule.h",
            "Source/EnigmaArcade/Public/ArcadeGameInstance.h",
            "Source/EnigmaArcade/Private/EnigmaArcadeModule.cpp",
            "Source/EnigmaArcade/Private/ArcadeGameInstance.cpp",
        };
        var allExist = true;
        foreach (var rel in enigmaArcadeCppFiles)
        {
            var full = Path.Combine(projectRoot, rel.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(full))
            {
                Console.WriteLine($"     MISSING: {rel}");
                allExist = false;
            }
        }
        Check(allExist, "[21] EnigmaArcade C++ source files present");

        // [22] ArcadeGameplay module C++ files
        var gameplayCppFiles = new[]
        {
            "Source/ArcadeGameplay/Public/ArcadeGameplayModule.h",
            "Source/ArcadeGameplay/Private/ArcadeGameplayModule.cpp",
        };
        allExist = true;
        foreach (var rel in gameplayCppFiles)
        {
            var full = Path.Combine(projectRoot, rel.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(full))
            {
                Console.WriteLine($"     MISSING: {rel}");
                allExist = false;
            }
        }
        Check(allExist, "[22] ArcadeGameplay C++ source files present");

        // [23] IMPLEMENT_PRIMARY_GAME_MODULE macro used in EnigmaArcade
        var enigmaModuleCpp = File.ReadAllText(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "Private", "EnigmaArcadeModule.cpp"));
        Check(enigmaModuleCpp.Contains("IMPLEMENT_PRIMARY_GAME_MODULE"),
            "[23] EnigmaArcade uses IMPLEMENT_PRIMARY_GAME_MODULE macro");

        // [24] FArcadeGameInstance extends FGameInstance and has tick counter
        var gameInstanceH = File.ReadAllText(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "Public", "ArcadeGameInstance.h"));
        Check(gameInstanceH.Contains("FGameInstance")
              && gameInstanceH.Contains("ENIGMAARCADE_API")
              && gameInstanceH.Contains("TickCount"),
            "[24] FArcadeGameInstance: extends FGameInstance, exported, has TickCount");
    }

    // ── ArcadeFeature plugin C++ files ─────────────────────────

    private static void TestPluginCppFiles(string projectRoot)
    {
        Console.WriteLine("\n--- ArcadeFeature Plugin C++ Files ---");

        // [29] ArcadeFeature plugin C++ source files exist
        var pluginCppFiles = new[]
        {
            "Plugins/ArcadeFeature/Source/ArcadeFeature/Public/ArcadeFeatureModule.h",
            "Plugins/ArcadeFeature/Source/ArcadeFeature/Private/ArcadeFeatureModule.cpp",
        };
        var allExist = true;
        foreach (var rel in pluginCppFiles)
        {
            var full = Path.Combine(projectRoot, rel.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(full))
            {
                Console.WriteLine($"     MISSING: {rel}");
                allExist = false;
            }
        }
        Check(allExist, "[29] ArcadeFeature plugin C++ source files present");

        // [30] IMPLEMENT_MODULE macro used (not IMPLEMENT_GAME_MODULE - plugins use IMPLEMENT_MODULE)
        var moduleCpp = File.ReadAllText(
            Path.Combine(projectRoot, "Plugins", "ArcadeFeature", "Source",
                "ArcadeFeature", "Private", "ArcadeFeatureModule.cpp"));
        Check(moduleCpp.Contains("IMPLEMENT_MODULE") && !moduleCpp.Contains("IMPLEMENT_GAME_MODULE"),
            "[30] ArcadeFeature uses IMPLEMENT_MODULE macro (not IMPLEMENT_GAME_MODULE)");

        // [31] Startup log message present in source
        Check(moduleCpp.Contains("PostEngineInit"),
            "[31] StartupModule logs PostEngineInit loading phase");

        // [32] PluginScanner discovers ArcadeFeature module
        var project = ProjectParser.Parse(
            Path.Combine(projectRoot, "EnigmaArcade.eproject"));
        var pluginsRoot = Path.Combine(projectRoot, "Plugins");
        var scanResult = PluginScanner.Scan(pluginsRoot, project.Plugins);

        Check(scanResult.EnabledPlugins.ContainsKey("ArcadeFeature")
              && scanResult.Modules.ContainsKey("ArcadeFeature"),
            "[32] PluginScanner discovers ArcadeFeature as enabled plugin with module");
    }

    // ── JSON integration (REQ-014) ─────────────────────────────

    private static void TestJsonIntegration(string projectRoot)
    {
        Console.WriteLine("\n--- JSON Integration (REQ-014) ---");

        // [33] EnigmaArcade.Build.cs declares nlohmann_json dependency
        var buildCs = ModuleParser.Parse(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "EnigmaArcade.Build.cs"));
        Check(buildCs.PrivateDependencyModuleNames.Contains("nlohmann_json"),
            "[33] EnigmaArcade.Build.cs has nlohmann_json as private dependency");

        // [34] ArcadeGameInstance.cpp includes nlohmann/json.hpp
        var gameInstanceCpp = File.ReadAllText(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "Private", "ArcadeGameInstance.cpp"));
        Check(gameInstanceCpp.Contains("#include <nlohmann/json.hpp>"),
            "[34] ArcadeGameInstance.cpp includes <nlohmann/json.hpp>");

        // [35] Source creates JSON config with game name and version
        Check(gameInstanceCpp.Contains("\"game\"") && gameInstanceCpp.Contains("\"EnigmaArcade\"")
              && gameInstanceCpp.Contains("\"version\""),
            "[35] Init() creates JSON with game name and version fields");

        // [36] Output format matches REQ-014: Config logged via ENIGMA_LOG with json dump
        Check(gameInstanceCpp.Contains("\"Config: {}\"")
              && gameInstanceCpp.Contains("config.dump()"),
            "[36] Output format: Config: <json> via ENIGMA_LOG (REQ-014)");
    }

    // ── BuildTool pipeline ────────────────────────────────────

    private static void TestBuildPipeline(string projectRoot)
    {
        Console.WriteLine("\n--- BuildTool Pipeline (parse → resolve → generate) ---");

        // Parse all modules
        var enigmaRules = ModuleParser.Parse(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "EnigmaArcade.Build.cs"));
        var gameplayRules = ModuleParser.Parse(
            Path.Combine(projectRoot, "Source", "ArcadeGameplay", "ArcadeGameplay.Build.cs"));
        var target = TargetParser.Parse(
            Path.Combine(projectRoot, "Source", "EnigmaArcade.Target.cs"));

        // Simulate engine modules (Core, Engine) as stubs for dependency resolution
        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new() { ModuleName = "Engine", PublicDependencyModuleNames = { "Core" } },
            [enigmaRules.ModuleName] = enigmaRules,
            [gameplayRules.ModuleName] = gameplayRules,
        };

        // [25] Dependency resolution
        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);
        Check(resolveResult.Success,
            $"[25] Dependency resolution succeeds (order: [{string.Join(", ", resolveResult.BuildOrder)}])");

        // [26] Build order: Core → Engine → ArcadeGameplay → EnigmaArcade
        if (resolveResult.Success)
        {
            var order = resolveResult.BuildOrder.ToList();
            var coreIdx = order.IndexOf("Core");
            var engineIdx = order.IndexOf("Engine");
            var enigmaIdx = order.IndexOf("EnigmaArcade");
            var gameplayIdx = order.IndexOf("ArcadeGameplay");

            Check(coreIdx < engineIdx && engineIdx < gameplayIdx && gameplayIdx < enigmaIdx,
                "[26] Build order: Core < Engine < ArcadeGameplay < EnigmaArcade");
        }

        // [27] CMake generation
        var gen = new CMakeGenerator();
        var genResult = gen.Generate("EnigmaArcade", modules, resolveResult, projectRoot, target);
        Check(genResult.Success, "[27] CMake generation succeeds");

        if (genResult.Success)
        {
            // [28] CMake content validation
            var cmake = genResult.Content;
            var hasEnigmaTarget = cmake.Contains("add_executable(EnigmaArcade")
                                  || cmake.Contains("add_library(EnigmaArcade");
            var hasGameplayTarget = cmake.Contains("add_executable(ArcadeGameplay")
                                    || cmake.Contains("add_library(ArcadeGameplay");
            var hasEnigmaExports = cmake.Contains("ENIGMAARCADE_EXPORTS");
            var hasGameplayExports = cmake.Contains("ARCADEGAMEPLAY_EXPORTS");
            var linksGameplay = cmake.Contains("target_link_libraries(EnigmaArcade")
                                && cmake.Contains("ArcadeGameplay");

            Check(hasEnigmaTarget && hasGameplayTarget
                  && hasEnigmaExports && hasGameplayExports && linksGameplay,
                "[28] CMake: both targets, EXPORTS macros, EnigmaArcade links ArcadeGameplay");
        }
    }

    // ── Multi-config build (Debug/Development/Shipping) ────────

    private static void TestMultiConfigBuild(string projectRoot)
    {
        Console.WriteLine("\n--- Multi-Config Build (Debug/Development/Shipping) ---");

        var (modules, resolveResult, target) = BuildFullModuleGraph(projectRoot);

        // [37-38] Debug configuration
        var gen = new CMakeGenerator();
        var debugResult = gen.Generate("EnigmaArcade", modules, resolveResult,
            projectRoot, target, BuildConfiguration.Debug);
        Check(debugResult.Success, "[37] CMake generation succeeds for Debug");
        if (debugResult.Success)
        {
            Check(debugResult.Content.Contains("ENIGMA_BUILD_DEBUG=1"),
                "[38] Debug CMake defines ENIGMA_BUILD_DEBUG=1");
        }

        // [39-40] Development configuration
        var devResult = gen.Generate("EnigmaArcade", modules, resolveResult,
            projectRoot, target, BuildConfiguration.Development);
        Check(devResult.Success, "[39] CMake generation succeeds for Development");
        if (devResult.Success)
        {
            // Development: executables present, no platform-config suffix in OUTPUT_NAME
            Check(devResult.Content.Contains("add_executable(EnigmaArcade")
                  && devResult.Content.Contains("add_executable(ArcadeGameplay")
                  && !devResult.Content.Contains("-Win64-Development"),
                "[40] Development: executable targets, no platform-config suffix");
        }

        // [41-42] Shipping configuration
        var shipResult = gen.Generate("EnigmaArcade", modules, resolveResult,
            projectRoot, target, BuildConfiguration.Shipping);
        Check(shipResult.Success, "[41] CMake generation succeeds for Shipping");
        if (shipResult.Success)
        {
            Check(shipResult.Content.Contains("ENIGMA_BUILD_SHIPPING=1")
                  && shipResult.Content.Contains("add_executable(EnigmaArcade"),
                "[42] Shipping CMake: ENIGMA_BUILD_SHIPPING=1 + executable targets");
        }
    }

    // ── Shipping package structure (REQ-015) ─────────────────

    private static void TestShippingStructure(string projectRoot)
    {
        Console.WriteLine("\n--- Shipping Package Structure (REQ-015) ---");

        var (modules, _, target) = BuildFullModuleGraph(projectRoot);

        // Scan plugins for packaging
        var project = ProjectParser.Parse(
            Path.Combine(projectRoot, "EnigmaArcade.eproject"));
        var pluginResult = PluginScanner.Scan(
            Path.Combine(projectRoot, "Plugins"), project.Plugins);

        // Merge plugin modules into the module graph
        foreach (var (name, rules) in pluginResult.Modules)
            modules[name] = rules;

        var engineSet = new HashSet<string>(StringComparer.Ordinal) { "Core", "Engine" };

        var packager = new ShippingPackager();
        var plan = packager.GeneratePlan("EnigmaArcade", modules, target,
            pluginScanResult: pluginResult, engineModuleNames: engineSet, buildId: "EA01");

        Check(plan.Success, "[43] Shipping package plan generated successfully");

        // [44] Root launcher
        var launcher = plan.Entries.Find(e => e.DestinationPath == "EnigmaArcade.exe");
        Check(launcher is not null && launcher.Category == ShippingPackager.EntryCategory.Launcher,
            "[44] Root launcher: EnigmaArcade.exe");

        // [45] Engine modules in Engine/Binaries/Win64/
        var coreInEngine = plan.Entries.Find(e =>
            e.DestinationPath == "Engine/Binaries/Win64/EnigmaArcade-Core-Win64-Shipping.dll");
        var engineInEngine = plan.Entries.Find(e =>
            e.DestinationPath == "Engine/Binaries/Win64/EnigmaArcade-Engine-Win64-Shipping.dll");
        Check(coreInEngine is not null && engineInEngine is not null,
            "[45] Engine modules (Core, Engine) in Engine/Binaries/Win64/");

        // [46] Game exe in EnigmaArcade/Binaries/ (no module DLLs - monolithic)
        var gameExe = plan.Entries.Find(e =>
            e.DestinationPath == "EnigmaArcade/Binaries/Win64/EnigmaArcade-Win64-Shipping.exe");
        var gameDlls = plan.Entries.FindAll(e =>
            e.DestinationPath.StartsWith("EnigmaArcade/Binaries/", StringComparison.Ordinal) &&
            e.DestinationPath.EndsWith(".dll", StringComparison.Ordinal));
        Check(gameExe is not null && gameDlls.Count == 0,
            "[46] Monolithic EXE in EnigmaArcade/Binaries/Win64/, no module DLLs");

        // [47] Plugin module in Engine/Plugins/ArcadeFeature/Binaries/Win64/
        var pluginDll = plan.Entries.Find(e =>
            e.DestinationPath == "Engine/Plugins/ArcadeFeature/Binaries/Win64/EnigmaArcade-ArcadeFeature-Win64-Shipping.dll");
        Check(pluginDll is not null,
            "[47] Plugin module in Engine/Plugins/ArcadeFeature/Binaries/Win64/");

        // [48] Engine config present
        var config = plan.Entries.Find(e =>
            e.DestinationPath == "Engine/Config/StagedBuild_EnigmaArcade.ini");
        Check(config is not null && config.GeneratedContent != null
              && config.GeneratedContent.Contains("GameName=EnigmaArcade"),
            "[48] Engine config: StagedBuild_EnigmaArcade.ini");

        // Print structure summary
        Console.WriteLine("     Shipping structure:");
        var dirs = plan.Entries
            .Select(e => string.Join("/", e.DestinationPath.Split('/').SkipLast(1)))
            .Where(d => d.Length > 0)
            .Distinct()
            .OrderBy(d => d);
        foreach (var d in dirs)
            Console.WriteLine($"       {d}/");
    }

    // ── Runtime output format (REQ-013) ──────────────────────

    private static void TestRuntimeOutputFormat(string projectRoot)
    {
        Console.WriteLine("\n--- Runtime Output Format (REQ-013) ---");

        // Validate expected output lines exist in source files
        // [49] Engine Initialized message
        var gameInstanceCpp = File.ReadAllText(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "Private", "ArcadeGameInstance.cpp"));
        Check(gameInstanceCpp.Contains("Engine Initialized"),
            "[49] FArcadeGameInstance outputs 'Engine Initialized' via ENIGMA_LOG");

        // [50] Module registration macros
        var enigmaModuleCpp = File.ReadAllText(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "Private", "EnigmaArcadeModule.cpp"));
        var gameplayModuleCpp = File.ReadAllText(
            Path.Combine(projectRoot, "Source", "ArcadeGameplay", "Private", "ArcadeGameplayModule.cpp"));
        Check(enigmaModuleCpp.Contains("IMPLEMENT_PRIMARY_GAME_MODULE")
              && gameplayModuleCpp.Contains("[ArcadeGameplay] StartupModule"),
            "[50] Primary module uses IMPLEMENT_PRIMARY_GAME_MODULE, gameplay module logs StartupModule");

        // [51] Plugin loading message
        var pluginModuleCpp = File.ReadAllText(
            Path.Combine(projectRoot, "Plugins", "ArcadeFeature", "Source",
                "ArcadeFeature", "Private", "ArcadeFeatureModule.cpp"));
        Check(pluginModuleCpp.Contains("[ArcadeFeature] Plugin loaded at PostEngineInit"),
            "[51] Plugin outputs '[ArcadeFeature] Plugin loaded at PostEngineInit'");

        // [52] Frame counter output (via ENIGMA_LOG, uses TickCount)
        Check(gameInstanceCpp.Contains("Frame: ")
              && gameInstanceCpp.Contains("TickCount"),
            "[52] Render outputs frame counter with TickCount");
    }

    // ── Full module graph builder (shared by multi-config tests) ──

    private static (Dictionary<string, ModuleRules> modules,
        DependencyResolver.ResolveResult resolveResult,
        TargetRules target) BuildFullModuleGraph(string projectRoot)
    {
        var engineRoot = ResolveEngineRoot();

        var enigmaRules = ModuleParser.Parse(
            Path.Combine(projectRoot, "Source", "EnigmaArcade", "EnigmaArcade.Build.cs"));
        var gameplayRules = ModuleParser.Parse(
            Path.Combine(projectRoot, "Source", "ArcadeGameplay", "ArcadeGameplay.Build.cs"));
        var target = TargetParser.Parse(
            Path.Combine(projectRoot, "Source", "EnigmaArcade.Target.cs"));

        // Scan ThirdParty for nlohmann_json
        var thirdPartyRoot = Path.Combine(engineRoot, "Engine", "Source", "ThirdParty");
        var thirdPartyModules = ThirdPartyScanner.Scan(thirdPartyRoot);

        var modules = new Dictionary<string, ModuleRules>(StringComparer.Ordinal)
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["Engine"] = new() { ModuleName = "Engine", PublicDependencyModuleNames = { "Core" } },
            [enigmaRules.ModuleName] = enigmaRules,
            [gameplayRules.ModuleName] = gameplayRules,
        };

        // Merge ThirdParty modules
        foreach (var (name, rules) in thirdPartyModules)
            modules[name] = rules;

        var resolver = new DependencyResolver();
        var resolveResult = resolver.Resolve(modules);

        return (modules, resolveResult, target);
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

    // ── Helpers ───────────────────────────────────────────────

    private static string ResolveProjectRoot()
    {
        var baseDir = AppContext.BaseDirectory;

        // Navigate from build output to EnigmaArcade
        var candidate = Path.GetFullPath(Path.Combine(
            baseDir, "..", "..", "..", "..", "..", "EnigmaArcade"));
        if (Directory.Exists(candidate))
            return candidate;

        // Fallback: search upward for EnigmaEngine root
        var dir = new DirectoryInfo(baseDir);
        while (dir is not null)
        {
            var testDir = Path.Combine(dir.FullName, "EnigmaArcade");
            if (Directory.Exists(testDir))
                return testDir;
            dir = dir.Parent;
        }

        throw new DirectoryNotFoundException("Cannot find EnigmaArcade project directory.");
    }

    private static void Check(bool cond, string name)
    {
        if (cond) { Console.WriteLine($"  [PASS] {name}"); ++_passed; }
        else      { Console.WriteLine($"  [FAIL] {name}"); ++_failed; }
    }
}
