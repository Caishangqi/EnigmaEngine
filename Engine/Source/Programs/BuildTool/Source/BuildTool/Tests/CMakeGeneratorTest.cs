using BuildTool.Analysis;
using BuildTool.Generators;
using BuildTool.Models;

namespace BuildTool.Tests;

/// <summary>
/// Smoke tests for CMakeGenerator.
/// </summary>
public static class CMakeGeneratorTest
{
    public static void Run()
    {
        Console.WriteLine("=== CMakeGenerator Smoke Tests ===");
        Console.WriteLine();

        TestSingleModule();
        TestMultiModuleWithDependencies();
        TestExportsMacro();
        TestIncludePaths();
        TestFailsOnUnresolvedDeps();
        TestFailsOnEmptyModules();
        TestDebugConfiguration();
        TestDebugGameConfiguration();
        TestDevelopmentConfiguration();
        TestShippingConfiguration();
        TestTestConfiguration();
        TestDllNamingDevelopment();
        TestDllNamingNonDevelopment();
        TestSharedLibsOnProducesDlls();
        TestStaticLibsOffProducesStaticLink();

        Console.WriteLine();
        Console.WriteLine("=== All tests passed ===");
    }

    /// <summary>
    /// Single module with no dependencies.
    /// Should produce a valid CMakeLists.txt with one library target (type controlled by BUILD_SHARED_LIBS).
    /// </summary>
    private static void TestSingleModule()
    {
        Console.WriteLine("[Test 1] Single module (Core, no deps)");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate("TestProject", modules, resolve, "C:/Projects/TestProject");

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        Assert(result.Content.Contains("cmake_minimum_required(VERSION 3.28)"), "Should contain cmake_minimum_required");
        Assert(result.Content.Contains("project(TestProject LANGUAGES CXX)"), "Should contain project()");
        Assert(result.Content.Contains("set(CMAKE_CXX_STANDARD 23)"), "Should set C++23 base standard");
        Assert(result.Content.Contains("add_library(Core ${CORE_SOURCES})"), "Should have Core library target");
        Assert(!result.Content.Contains("target_link_libraries(Core"), "Core should have no link deps");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Multi-module: Launch -> Engine -> Core.
    /// Core should appear before Engine, Engine before Launch.
    /// </summary>
    private static void TestMultiModuleWithDependencies()
    {
        Console.WriteLine("[Test 2] Multi-module with dependencies");

        var modules = new Dictionary<string, ModuleRules>
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
                PrivateDependencyModuleNames = { "Core" },
            },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate("MyGame", modules, resolve, "D:\\Projects\\MyGame");

        Assert(result.Success, $"Expected success, got error: {result.Error}");

        // Core target must appear before Engine, Engine before Launch in the output
        var corePos = result.Content.IndexOf("add_library(Core ${CORE_SOURCES})", StringComparison.Ordinal);
        var enginePos = result.Content.IndexOf("add_library(Engine ${ENGINE_SOURCES})", StringComparison.Ordinal);
        var launchPos = result.Content.IndexOf("add_library(Launch ${LAUNCH_SOURCES})", StringComparison.Ordinal);

        Assert(corePos >= 0, "Core target should exist");
        Assert(enginePos >= 0, "Engine target should exist");
        Assert(launchPos >= 0, "Launch target should exist");
        Assert(corePos < enginePos, "Core should appear before Engine");
        Assert(enginePos < launchPos, "Engine should appear before Launch");

        // Verify dependency linking
        Assert(result.Content.Contains("target_link_libraries(Engine"), "Engine should have link deps");
        Assert(result.Content.Contains("target_link_libraries(Launch"), "Launch should have link deps");

        // Path normalization: backslashes should become forward slashes
        Assert(!result.Content.Contains("D:\\"), "Paths should use forward slashes");
        Assert(result.Content.Contains("D:/Projects/MyGame"), "Normalized path should be present");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Verify {MODULE}_EXPORTS macro is defined for each module.
    /// </summary>
    private static void TestExportsMacro()
    {
        Console.WriteLine("[Test 3] EXPORTS macro definitions");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["RenderCore"] = new()
            {
                ModuleName = "RenderCore",
                PublicDependencyModuleNames = { "Core" },
            },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate("TestProject", modules, resolve, "/home/dev/project");

        Assert(result.Success, $"Expected success, got error: {result.Error}");

        // Each module should have its EXPORTS macro defined as PRIVATE compile definition
        Assert(result.Content.Contains("target_compile_definitions(Core PRIVATE CORE_EXPORTS)"),
            "Core should define CORE_EXPORTS");
        Assert(result.Content.Contains("target_compile_definitions(RenderCore PRIVATE RENDERCORE_EXPORTS)"),
            "RenderCore should define RENDERCORE_EXPORTS");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Verify include paths are correctly generated.
    /// </summary>
    private static void TestIncludePaths()
    {
        Console.WriteLine("[Test 4] Include paths");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["MyModule"] = new()
            {
                ModuleName = "MyModule",
                PublicIncludePaths = { "Public", "Public/API" },
                PrivateIncludePaths = { "Private", "Private/Internal" },
            },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate("TestProject", modules, resolve, "/project");

        Assert(result.Success, $"Expected success, got error: {result.Error}");

        // Default Public/Private paths should be present
        Assert(result.Content.Contains("MyModule/Public\""), "Should include Public path");
        Assert(result.Content.Contains("MyModule/Private\""), "Should include Private path");

        // Custom paths should also be present
        Assert(result.Content.Contains("MyModule/Public/API\""), "Should include Public/API path");
        Assert(result.Content.Contains("MyModule/Private/Internal\""), "Should include Private/Internal path");

        // "Public" should not be duplicated (it's both default and in PublicIncludePaths)
        var content = result.Content;
        var firstPublic = content.IndexOf("MyModule/Public\"", StringComparison.Ordinal);
        var secondPublic = content.IndexOf("MyModule/Public\"", firstPublic + 1, StringComparison.Ordinal);
        Assert(secondPublic == -1, "Public path should not be duplicated");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Generation should fail if dependency resolution failed.
    /// </summary>
    private static void TestFailsOnUnresolvedDeps()
    {
        Console.WriteLine("[Test 5] Fails on unresolved dependencies (cycle)");

        // Create a cyclic dependency: A -> B -> A
        var modules = new Dictionary<string, ModuleRules>
        {
            ["A"] = new() { ModuleName = "A", PublicDependencyModuleNames = { "B" } },
            ["B"] = new() { ModuleName = "B", PublicDependencyModuleNames = { "A" } },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        Assert(!resolve.Success, "Resolve should fail for cyclic deps");

        var result = new CMakeGenerator().Generate("TestProject", modules, resolve, "/project");

        Assert(!result.Success, "Generate should fail when resolve failed");
        Assert(result.Error!.Contains("dependency resolution failed"), $"Error should mention resolution failure, got: {result.Error}");
        Assert(string.IsNullOrEmpty(result.Content), "Content should be empty on failure");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Generation should fail if no modules provided.
    /// </summary>
    private static void TestFailsOnEmptyModules()
    {
        Console.WriteLine("[Test 6] Fails on empty modules");

        var modules = new Dictionary<string, ModuleRules>();
        var resolve = new DependencyResolver().Resolve(modules);

        var result = new CMakeGenerator().Generate("TestProject", modules, resolve, "/project");

        Assert(!result.Success, "Generate should fail for empty modules");
        Assert(result.Error!.Contains("no modules"), $"Error should mention no modules, got: {result.Error}");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Debug configuration: no optimization, debug symbols, ENIGMA_BUILD_DEBUG=1, no NDEBUG.
    /// </summary>
    private static void TestDebugConfiguration()
    {
        Console.WriteLine("[Test 7] Debug configuration");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "TestProject", modules, resolve, "/project",
            configuration: BuildConfiguration.Debug);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        // Config identification macro
        Assert(c.Contains("ENIGMA_BUILD_DEBUG=1"), "Debug should define ENIGMA_BUILD_DEBUG=1");
        Assert(!c.Contains("ENIGMA_BUILD_SHIPPING"), "Debug should NOT define ENIGMA_BUILD_SHIPPING");
        Assert(!c.Contains("ENIGMA_BUILD_DEVELOPMENT"), "Debug should NOT define ENIGMA_BUILD_DEVELOPMENT");
        Assert(!c.Contains("NDEBUG"), "Debug should NOT define NDEBUG");

        // Optimization: /Od (MSVC), -O0 (GCC)
        Assert(c.Contains("/Od"), "Debug should use /Od");
        Assert(c.Contains("-O0"), "Debug should use -O0");

        // Debug symbols
        Assert(c.Contains("/Zi"), "Debug should enable /Zi");
        Assert(c.Contains("/DEBUG"), "Debug should enable /DEBUG linker flag");
        Assert(c.Contains("add_compile_options(-g)"), "Debug should enable -g");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// DebugGame configuration: moderate optimization, debug symbols, ENIGMA_BUILD_DEBUGGAME=1.
    /// </summary>
    private static void TestDebugGameConfiguration()
    {
        Console.WriteLine("[Test 8] DebugGame configuration");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "TestProject", modules, resolve, "/project",
            configuration: BuildConfiguration.DebugGame);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        Assert(c.Contains("ENIGMA_BUILD_DEBUGGAME=1"), "DebugGame should define ENIGMA_BUILD_DEBUGGAME=1");
        Assert(!c.Contains("ENIGMA_BUILD_SHIPPING"), "DebugGame should NOT define ENIGMA_BUILD_SHIPPING");
        Assert(!c.Contains("NDEBUG"), "DebugGame should NOT define NDEBUG");

        // Optimization: /O1 (MSVC), -O1 (GCC)
        Assert(c.Contains("/O1"), "DebugGame should use /O1");
        Assert(c.Contains("-O1"), "DebugGame should use -O1");

        // Debug symbols
        Assert(c.Contains("/Zi"), "DebugGame should enable /Zi");
        Assert(c.Contains("add_compile_options(-g)"), "DebugGame should enable -g");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Development configuration: moderate optimization, debug symbols, ENIGMA_BUILD_DEVELOPMENT=1.
    /// </summary>
    private static void TestDevelopmentConfiguration()
    {
        Console.WriteLine("[Test 9] Development configuration");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "TestProject", modules, resolve, "/project",
            configuration: BuildConfiguration.Development);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        Assert(c.Contains("ENIGMA_BUILD_DEVELOPMENT=1"), "Development should define ENIGMA_BUILD_DEVELOPMENT=1");
        Assert(!c.Contains("ENIGMA_BUILD_SHIPPING"), "Development should NOT define ENIGMA_BUILD_SHIPPING");
        Assert(!c.Contains("NDEBUG"), "Development should NOT define NDEBUG");

        // Optimization: /O1 (MSVC), -O1 (GCC)
        Assert(c.Contains("/O1"), "Development should use /O1");
        Assert(c.Contains("-O1"), "Development should use -O1");

        // Debug symbols
        Assert(c.Contains("/Zi"), "Development should enable /Zi");
        Assert(c.Contains("add_compile_options(-g)"), "Development should enable -g");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Shipping configuration: full optimization, NO debug symbols, ENIGMA_BUILD_SHIPPING=1, NDEBUG.
    /// </summary>
    private static void TestShippingConfiguration()
    {
        Console.WriteLine("[Test 10] Shipping configuration");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "TestProject", modules, resolve, "/project",
            configuration: BuildConfiguration.Shipping);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        Assert(c.Contains("ENIGMA_BUILD_SHIPPING=1"), "Shipping should define ENIGMA_BUILD_SHIPPING=1");
        Assert(c.Contains("NDEBUG"), "Shipping should define NDEBUG");
        Assert(!c.Contains("ENIGMA_BUILD_DEBUG=1"), "Shipping should NOT define ENIGMA_BUILD_DEBUG");
        Assert(!c.Contains("ENIGMA_BUILD_DEVELOPMENT=1"), "Shipping should NOT define ENIGMA_BUILD_DEVELOPMENT");

        // Full optimization: /O2 (MSVC), -O3 (GCC)
        Assert(c.Contains("/O2"), "Shipping should use /O2");
        Assert(c.Contains("-O3"), "Shipping should use -O3");

        // NO debug symbols
        Assert(!c.Contains("/Zi"), "Shipping should NOT enable /Zi");
        Assert(!c.Contains("/DEBUG"), "Shipping should NOT enable /DEBUG");
        Assert(!c.Contains("add_compile_options(-g)"), "Shipping should NOT enable -g");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Test configuration: full optimization like Shipping, but WITH debug symbols. NDEBUG defined.
    /// </summary>
    private static void TestTestConfiguration()
    {
        Console.WriteLine("[Test 11] Test configuration");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "TestProject", modules, resolve, "/project",
            configuration: BuildConfiguration.Test);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        Assert(c.Contains("ENIGMA_BUILD_TEST=1"), "Test should define ENIGMA_BUILD_TEST=1");
        Assert(c.Contains("NDEBUG"), "Test should define NDEBUG");
        Assert(!c.Contains("ENIGMA_BUILD_SHIPPING"), "Test should NOT define ENIGMA_BUILD_SHIPPING");

        // Full optimization: /O2 (MSVC), -O3 (GCC)
        Assert(c.Contains("/O2"), "Test should use /O2");
        Assert(c.Contains("-O3"), "Test should use -O3");

        // Debug symbols (unlike Shipping)
        Assert(c.Contains("/Zi"), "Test should enable /Zi");
        Assert(c.Contains("/DEBUG"), "Test should enable /DEBUG");
        Assert(c.Contains("add_compile_options(-g)"), "Test should enable -g");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Development configuration uses short DLL name: {EnigmaEngine}-{ModuleName}
    /// (non-Shipping builds use fixed "EnigmaEngine" prefix instead of project name)
    /// </summary>
    private static void TestDllNamingDevelopment()
    {
        Console.WriteLine("[Test 12] DLL naming: Development uses short name");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["ArcadeCore"] = new()
            {
                ModuleName = "ArcadeCore",
                PublicDependencyModuleNames = { "Core" },
            },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate(
            "EnigmaArcade", modules, resolve, "/project",
            configuration: BuildConfiguration.Development);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        // Development: OUTPUT_NAME = "{EnigmaEngine}-{ModuleName}"
        Assert(c.Contains("OUTPUT_NAME \"EnigmaEngine-Core\""),
            "Development Core should be EnigmaEngine-Core");
        Assert(c.Contains("OUTPUT_NAME \"EnigmaEngine-ArcadeCore\""),
            "Development ArcadeCore should be EnigmaEngine-ArcadeCore");

        // Should NOT contain platform or config suffix
        Assert(!c.Contains("Win64-Development"), "Development should not have platform-config suffix");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Non-Development configurations use full DLL name: {Prefix}-{ModuleName}-{Platform}-{Config}
    /// Shipping uses project name as prefix; other non-Development configs use "EnigmaEngine".
    /// </summary>
    private static void TestDllNamingNonDevelopment()
    {
        Console.WriteLine("[Test 13] DLL naming: non-Development uses full name");

        var modules = new Dictionary<string, ModuleRules>
        {
            ["Core"] = new() { ModuleName = "Core" },
            ["ArcadeCore"] = new()
            {
                ModuleName = "ArcadeCore",
                PublicDependencyModuleNames = { "Core" },
            },
        };

        var resolve = new DependencyResolver().Resolve(modules);

        // Test all 4 non-Development configurations
        var configs = new[]
        {
            (BuildConfiguration.Debug, "Debug"),
            (BuildConfiguration.DebugGame, "DebugGame"),
            (BuildConfiguration.Shipping, "Shipping"),
            (BuildConfiguration.Test, "Test"),
        };

        foreach (var (config, configName) in configs)
        {
            var result = new CMakeGenerator().Generate(
                "EnigmaArcade", modules, resolve, "/project",
                configuration: config);

            Assert(result.Success, $"Expected success for {configName}, got error: {result.Error}");
            var c = result.Content;

            // Shipping uses project name, others use engine name
            var prefix = config == BuildConfiguration.Shipping ? "EnigmaArcade" : "EnigmaEngine";
            Assert(c.Contains($"OUTPUT_NAME \"{prefix}-Core-Win64-{configName}\""),
                $"{configName} Core should be {prefix}-Core-Win64-{configName}");
            Assert(c.Contains($"OUTPUT_NAME \"{prefix}-ArcadeCore-Win64-{configName}\""),
                $"{configName} ArcadeCore should be {prefix}-ArcadeCore-Win64-{configName}");
        }

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Verify add_library calls do NOT contain hardcoded SHARED keyword.
    /// Library type is controlled by BUILD_SHARED_LIBS at configure time.
    /// </summary>
    private static void TestSharedLibsOnProducesDlls()
    {
        Console.WriteLine("[Test 14] BUILD_SHARED_LIBS: no hardcoded SHARED in add_library");

        var modules = new Dictionary<string, ModuleRules>
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

        var target = new TargetRules
        {
            TargetName = "TestGame",
            Type = TargetType.Game,
            ExtraModuleNames = { "Launch" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate("TestGame", modules, resolve, "/project", target);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        // add_library should NOT have hardcoded SHARED
        Assert(!c.Contains("add_library(Core SHARED"), "Core should not have hardcoded SHARED");
        Assert(!c.Contains("add_library(Engine SHARED"), "Engine should not have hardcoded SHARED");

        // add_library should use plain form (BUILD_SHARED_LIBS controls type)
        Assert(c.Contains("add_library(Core ${CORE_SOURCES})"), "Core should use plain add_library");
        Assert(c.Contains("add_library(Engine ${ENGINE_SOURCES})"), "Engine should use plain add_library");

        // Launch is executable - should remain add_executable
        Assert(c.Contains("add_executable(Launch ${LAUNCH_SOURCES})"), "Launch should be add_executable");

        // DLL naming should be conditional on BUILD_SHARED_LIBS
        Assert(c.Contains("if(BUILD_SHARED_LIBS)"), "DLL naming should be wrapped in if(BUILD_SHARED_LIBS)");

        Console.WriteLine("  PASSED");
    }

    /// <summary>
    /// Verify output contains monolithic linking block: if(NOT BUILD_SHARED_LIBS) with
    /// target_link_libraries linking all library modules to Launch, and STATIC_DEFINE definitions.
    /// </summary>
    private static void TestStaticLibsOffProducesStaticLink()
    {
        Console.WriteLine("[Test 15] Monolithic: STATIC_DEFINE and target_link_libraries block");

        var modules = new Dictionary<string, ModuleRules>
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

        var target = new TargetRules
        {
            TargetName = "TestGame",
            Type = TargetType.Game,
            ExtraModuleNames = { "Launch" },
        };

        var resolve = new DependencyResolver().Resolve(modules);
        var result = new CMakeGenerator().Generate("TestGame", modules, resolve, "/project", target);

        Assert(result.Success, $"Expected success, got error: {result.Error}");
        var c = result.Content;

        // STATIC_DEFINE for each library module
        Assert(c.Contains("CORE_STATIC_DEFINE"),
            "Core should have CORE_STATIC_DEFINE");
        Assert(c.Contains("ENGINE_STATIC_DEFINE"),
            "Engine should have ENGINE_STATIC_DEFINE");
        Assert(c.Contains("target_compile_definitions(Core PUBLIC CORE_STATIC_DEFINE)"),
            "Core STATIC_DEFINE should be PUBLIC");
        Assert(c.Contains("target_compile_definitions(Engine PUBLIC ENGINE_STATIC_DEFINE)"),
            "Engine STATIC_DEFINE should be PUBLIC");

        // Monolithic linking block: Launch links all library modules
        Assert(c.Contains("# ── Monolithic (static) linking ──"),
            "Should have monolithic linking comment");
        Assert(c.Contains("target_link_libraries(Launch"),
            "Should have monolithic target_link_libraries for Launch");

        // Verify the monolithic block is inside if(NOT BUILD_SHARED_LIBS)
        int monolithicIfPos = c.IndexOf("if(NOT BUILD_SHARED_LIBS)", c.IndexOf("Monolithic"), StringComparison.Ordinal);
        int monolithicLinkPos = c.IndexOf("target_link_libraries(Launch", monolithicIfPos, StringComparison.Ordinal);
        Assert(monolithicIfPos >= 0, "Monolithic block should have if(NOT BUILD_SHARED_LIBS)");
        Assert(monolithicLinkPos > monolithicIfPos, "target_link_libraries should be inside the if block");

        // WHOLE_ARCHIVE: library modules must use WHOLE_ARCHIVE to prevent
        // linker dead-stripping of IMPLEMENT_MODULE static registrars
        Assert(c.Contains("$<LINK_LIBRARY:WHOLE_ARCHIVE,Core>"),
            "Core should be linked with WHOLE_ARCHIVE");
        Assert(c.Contains("$<LINK_LIBRARY:WHOLE_ARCHIVE,Engine>"),
            "Engine should be linked with WHOLE_ARCHIVE");

        // Launch (executable) should NOT have STATIC_DEFINE
        Assert(!c.Contains("LAUNCH_STATIC_DEFINE"),
            "Executable Launch should not have STATIC_DEFINE");

        Console.WriteLine("  PASSED");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception($"Assertion failed: {message}");
        }
    }
}
