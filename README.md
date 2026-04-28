<p align="center"><img src="https://github.com/user-attachments/assets/12663581-fb81-4364-96d6-36e57d6cfd4f" alt="Logo" width="300"></p>

<h1 align="center"> Enigma Engine </h1>
<h4 align="center">A modern, progressive, lightweight modular game engine for voxel game development</h4>
<p align="center">
<a href="https://www.codefactor.io/repository/github/caishangqi/EnigmaEngine"><img src="https://www.codefactor.io/repository/github/caishangqi/EnigmaEngine/badge" alt="CodeFactor" /></a>
<img alt="Renderer Backend" src="https://img.shields.io/badge/Render API-ASCII-242629">
<img alt="C++ Language Level" src="https://img.shields.io/badge/C++-26-cherry">
<img alt="Dot Net" src="https://img.shields.io/badge/.Net-9.0-573ccf">
<img alt="GitHub branch checks state" src="https://img.shields.io/github/checks-status/Caishangqi/EnigmaEngine/master?label=build">
<img alt="GitHub code size in bytes" src="https://img.shields.io/github/languages/code-size/Caishangqi/EnigmaEngine">
</p>

<p align="center"><a href="README_ZH.md">中文文档</a></p>

## Overview

Enigma Engine is a C++ game engine designed from the ground up for voxel game development. It features a modular architecture with runtime DLL loading/unloading, a plugin system, and multi-configuration builds. The engine ships with a complete project scaffolding toolchain -- from project creation to shipping -- all driven by the BuildTool CLI.

## Features

- Modular architecture: engine, game logic, and plugins run as independent DLL modules with manifest-based DLL loading for multi-configuration isolation, engine plugin auto-discovery, and dependency management
- Structured logging: category-based `ENIGMA_LOG` macro with compile-time verbosity gating (zero-overhead in Shipping), runtime level control, and automatic stderr/stdout routing
- Plugin system: plugins defined via `.eplugin` descriptors with loading phase control, auto-discovery, and dependency checking
- INI config system: UE-style layered config with 4-layer merging (Engine Base → Plugin → Project Default → User), typed getters/setters, array operators, plugin dual-track support
- Project scaffolding: one-command creation of projects, modules, and plugins with auto-generated template code and configuration
- Multi-configuration builds: Debug / DebugGame / Development / Shipping / Test
- Visual Studio / Rider integration: generated `.sln`, `.vcxproj`, engine-only workspace, and Rider run configurations
- AutomationTest framework: UE-style test authoring with a GoogleTest backend, BuildTool-driven discovery, filtering, execution, and JSON reports
- Unreal-style API: familiar naming conventions and architectural patterns (ModuleRules, TargetRules, GameInstance)
- Core math library: FVector, FMatrix, FQuat, FRotator, FTransform and more, right-hand Y-up coordinate system, constexpr-friendly
- Delegate & event system: type-safe TDelegate, TMulticastDelegate with FDelegateHandle lifecycle management, supports static/lambda/member function bindings
- Engine subsystem framework: extensible SubsystemCollection with automatic lifecycle management, similar to UE's subsystem architecture
- Async task infrastructure: FThreadPool for general-purpose thread pooling, FTaskGraph for dependency-based parallel task scheduling with named tasks and prerequisite chains
- Tick system: FTickTaskManager subsystem with tick groups (PreUpdate/Update/PostUpdate), prerequisite-based ordering, optional multi-threaded dispatch via FTaskGraph, and configurable tick intervals
- Game object framework: FScene / FGameObject / FComponent hierarchy with lifecycle hooks (OnAttach, BeginPlay, Update, OnDetach), scene-driven BeginPlay dispatch following UE5 patterns, and dynamic object support
- Enhanced Input system: action-based input with triggers (Pressed/Released/Down), modifiers (Negate/Swizzle/DeadZone/Scalar), and mapping contexts with priority support
- ASCII renderer: frame-buffer based ASCII art rendering with Z-depth sorting, scene view camera, Y-up coordinate convention, and VT100 terminal output
- Thread-safe ticker: FTSTicker frame-level callback system with thread-safe pending queue, supporting one-shot and recurring delegates
- Directory watcher: real-time file system monitoring via Windows overlapped I/O + APC, no background threads, integrated with FTSTicker for frame-driven polling
- DLL hot-reload: versioned DLL hot-reload system (-0001, -0002 suffixes) matching UE's pattern, with automatic detection when building from IDE while engine is running

## Planned Features

- Game Editor with full hot-reload integration for game module and plugin DLLs (currently runtime-only with GameInstance recreation workaround)
- Integration of `create-module`, `create-plugin` and other build actions into the Game Editor
- Decouple renderer initialization from window creation via a Viewport abstraction layer (similar to UE's RHI / GameViewport split), enabling multiple render backends (DX12, Vulkan)

## Building

### Prerequisites

- C++26 compiler (MSVC 17 2022 or later)
- CMake 3.20+
- .NET 9.0 SDK (for BuildTool)

### BuildTool

BuildTool is a C# .NET 9 CLI that handles project scanning, dependency resolution, CMake generation, compilation, and packaging.

```bash
# Build project (defaults to Development configuration)
BuildTool build <project-path>

# Specify build configuration
BuildTool build <project-path> -c DebugGame
BuildTool build <project-path> -c Shipping

# Clean / Rebuild
BuildTool clean <project-path>
BuildTool rebuild <project-path>

# Generate Visual Studio solution
BuildTool generate-project-files <project-path>

# Generate engine-only workspace
BuildTool generate-engine-project-files <repo-root>

# List / run automation tests
BuildTool automation-test <project-path> --list --profile all-non-perf
BuildTool automation-test <project-path> --run --profile local-fast --report Saved/AutomationReports
BuildTool automation-test <repo-root> --engine --run --profile ci-standard

# Package for distribution (forces Shipping configuration)
BuildTool package <project-path> -o <output-path>
```

### Automation Tests

New tests should live next to the module they validate:

```text
Engine/Source/Runtime/Core/Private/Tests/NameAutomationTests.cpp
Engine/Source/Developer/DirectoryWatcher/Private/Tests/DirectoryWatcherAutomationTests.cpp
EnigmaArcade/Source/EnigmaArcade/Private/Tests/MyGameAutomationTests.cpp
```

Modules with `Private/Tests` sources should declare test-only dependencies in their `.Build.cs` files:

```csharp
PrivateTestDependencyModuleNames.Add("AutomationTest");
```

The root `Tests/` directory is reserved for standalone build and module validation projects. Prefer module-local `Private/Tests/` for module and feature coverage.

### Project Scaffolding

```bash
# Create a new game project
BuildTool create-project --name MyGame --location .

# Create a game module
BuildTool create-module <project-path> --name GameUtils

# Create a plugin
BuildTool create-plugin <project-path> --name MyFeature --category Gameplay

# Remove a module (with dependency checking)
BuildTool remove-module <project-path> --name GameUtils

# Remove a plugin
BuildTool remove-plugin <project-path> --name MyFeature
```

### Build Configurations

| Configuration | Description | Link Mode | Optimization | EXE Location | Engine DLLs | Game DLLs |
|---------------|:-----------:|:---------:|:------------:|:------------:|:-----------:|:---------:|
| `Debug` | Full debug, no optimization | Modular (DLL) | /Od | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |
| `DebugGame` | Engine optimized, game debuggable | Modular (DLL) | /O1 | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |
| `Development` | Development build, moderate optimization | Modular (DLL) | /O1 | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |
| `Shipping` | Release build, full optimization | Monolithic (static) | /O2 | Project/Binaries/ | N/A | N/A |
| `Test` | Automated test build | Modular (DLL) | /O2 | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |

### Binary Layout

Modular builds (Debug / DebugGame / Development) follow UE's split layout:

```
Engine/Binaries/Win64/           EXE + engine module DLLs + .modules manifest
{Project}/Binaries/Win64/        Game module DLLs + .modules + .target manifest
{Project}/Plugins/{Name}/Binaries/Win64/   Plugin DLLs + .modules manifest
```

Shipping builds produce a single monolithic executable:

```
{Project}/Binaries/Win64/        Monolithic EXE + .target manifest
```

The EXE locates game DLLs at runtime via `--project-dir=` command line argument (auto-configured in VS debugger settings).

## Continuous Integration

CI builds BuildTool, runs BuildTool validation suites, runs engine AutomationTest with the `ci-standard` profile, keeps standalone module validation projects green, and verifies the example project build/package flow.

## Modules

| **Name** | **Description** | **Status** |
|----------|:---------------:|:----------:|
| `Enigma::Core` | Foundation types, modules, logging, config, delegates, math, async tasks, and ticker | stable |
| `Enigma::ApplicationCore` | Cross-platform application, window, and message-pump abstraction | stable |
| `Enigma::RenderCore` | Renderer-facing abstraction layer | stable |
| `Enigma::AsciiRenderer` | ASCII frame-buffer renderer and scene view output | stable |
| `Enigma::Engine` | Engine loop, game instance, subsystems, tick, scene, object, and component runtime | stable |
| `Enigma::Launch` | Executable entry point and guarded startup | stable |
| `Enigma::EnhancedInput` | Action-based input plugin | stable |
| `Enigma::DirectoryWatcher` | Developer file-system watcher | stable |
| `Enigma::HotReload` | Developer DLL hot-reload support | stable |
| `Enigma::AutomationTest` | Developer automation test API and registry | experimental |

## Third Party

| **Name** | **Description** | **Link** |
|----------|:---------------:|:--------:|
| `nlohmann::json` | JSON for Modern C++ | [Github](https://github.com/nlohmann/json) |
| `Google Test` | AutomationTest execution backend (git submodule) | [Github](https://github.com/google/googletest) |

## Project Structure

```
EnigmaEngine/
  Engine/
    Binaries/Win64/              Engine DLLs + EXE (Modular builds)
    Config/                      Engine base config (BaseEngine.ini, BaseGame.ini)
    Intermediate/                Engine build intermediates + generated .vcxproj files
    Source/
      Runtime/                   Runtime modules
        Core/                      Foundation: module system, logging, math, delegates, config, async tasks
        ApplicationCore/           Platform application & window abstraction (Win32)
        RenderCore/                Renderer interface abstraction (IRendererModule)
        AsciiRenderer/             ASCII frame-buffer renderer with Z-depth & scene camera
        Engine/                    Engine loop, GameEngine, GameInstance, SubsystemCollection, TickSystem, Scene/GameObject/Component
        Launch/                    Entry point (GuardedMain, main/WinMain)
      Developer/                 Developer tools (excluded from Shipping builds)
        AutomationTest/            Automation test API and registry
        DirectoryWatcher/          Real-time file system monitoring (Windows overlapped I/O)
        HotReload/                 Versioned DLL hot-reload system
      ThirdParty/                Third-party libraries (nlohmann_json, googletest)
      Programs/
        AutomationTestRunner/     Standalone automation test runner
        BuildTool/               C# .NET 9 CLI build tool
    Plugins/
      EnhancedInput/             Engine plugin: action-based input system
        EnhancedInput.eplugin      Plugin descriptor
        Source/EnhancedInput/      Module source (Public/ + Private/)
        Binaries/                  Plugin DLLs (Modular builds)
    Templates/                   Scaffolding templates (Project, Module, Plugin)
  EnigmaArcade/                  Example game project
    EnigmaArcade.eproject        Project descriptor
    Config/                      Project config (DefaultEngine.ini, DefaultGame.ini)
    Source/
      EnigmaArcade/              Main game module
    Plugins/
      ArcadeFeature/             Game plugin
        ArcadeFeature.eplugin      Plugin descriptor
        Config/                    Plugin config (DefaultArcadeFeature.ini)
        Source/ArcadeFeature/      Module source (Public/ + Private/)
    Binaries/Win64/              Game DLLs (Modular) or monolithic EXE (Shipping)
    Intermediate/                Build intermediates + generated .vcxproj files
  .clang-format                  C++ formatting rules
  .editorconfig                  Editor and C# analysis rules
  Tests/
    CoreModuleTest/              Core module standalone validation
    DllExportMacroTest/          DLL export macro validation
    EngineLoopTest/              Engine loop standalone validation
    EngineModuleTest/            Engine module standalone validation
    GameEngineConnectionTest/    Game engine connection validation
    GameEngineTest/              Game engine standalone validation
    GameInstanceTest/            Game instance standalone validation
    ModuleInterfaceTest/         Module interface integration test
    ModuleManagerTest/           Module manager integration test
    LaunchModuleTest/            Launch module integration test
    NlohmannJsonTest/            nlohmann_json integration test
    NlohmannJsonIntegrationTest/ nlohmann_json BuildTool project integration
```

<p>&nbsp;
</p>

<p align="center">
<a href="https://github.com/Caishangqi/EnigmaEngine/issues">
<img src="https://i.imgur.com/qPmjSXy.png" width="160" />
</a>
<a href="https://github.com/Caishangqi/EnigmaEngine">
<img src="https://i.imgur.com/L1bU9mr.png" width="160" />
</a>
<a href="[https://discord.gg/3rPcYrPnAs](https://discord.gg/3rPcYrPnAs)">
<img src="https://i.imgur.com/uf6V9ZX.png" width="160" />
</a>
<a href="https://github.com/Caishangqi">
<img src="https://i.imgur.com/fHQ45KR.png" width="227" />
</a>
</p>

<h1></h1>
<h4 align="center">Find out more about EnigmaEngine on the <a href="https://github.com/Caishangqi">SMU Pages</a></h4>
<h4 align="center">Looking for the custom support? <a href="https://github.com/Caishangqi">Find it here</a></h4>
