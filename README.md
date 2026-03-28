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
- Visual Studio integration: auto-generated `.sln` and `.vcxproj` project files
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

# Package for distribution (forces Shipping configuration)
BuildTool package <project-path> -o <output-path>
```

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

## Modules

| **Name** | **Description** | **Status** |
|----------|:---------------:|:----------:|
| `Enigma::Core` | Foundation module providing the module system, logging, assertions, HAL platform abstraction, delegate/event system (TDelegate, TMulticastDelegate), INI config system (FConfigCacheIni, GConfig), core math types (FVector, FMatrix, FQuat, FRotator, FTransform), async task infrastructure (FThreadPool, FTaskGraph), and FTSTicker thread-safe frame ticker | stable |
| `Enigma::ApplicationCore` | Platform-agnostic application and window abstraction (FGenericApplication, FGenericWindow, FGenericApplicationMessageHandler) with Win32 implementation | stable |
| `Enigma::RenderCore` | Renderer interface abstraction layer (IRendererModule) decoupling engine from concrete renderer implementations | stable |
| `Enigma::AsciiRenderer` | ASCII art renderer with frame-buffer, Z-depth sorting, scene view camera, and VT100 terminal output | stable |
| `Enigma::Engine` | Engine core providing FEngineLoop, FGameEngine with config-driven window creation, FGameInstance, SubsystemCollection, FTickTaskManager tick scheduling, FScene/FGameObject/FComponent game object framework with scene-driven BeginPlay lifecycle, and module loading phase management | stable |
| `Enigma::Launch` | Entry point module providing GuardedMain and platform-specific launch logic (main / WinMain) | stable |
| `Enigma::EnhancedInput` | Action-based input system with triggers, modifiers, and mapping contexts (engine plugin) | stable |
| `Enigma::DirectoryWatcher` | Real-time file system monitoring via Windows overlapped I/O + APC, integrated with FTSTicker (developer tool, excluded from Shipping) | stable |
| `Enigma::HotReload` | Versioned DLL hot-reload system matching UE's pattern, with automatic IDE build detection (developer tool, excluded from Shipping) | stable |

## Third Party

| **Name** | **Description** | **Link** |
|----------|:---------------:|:--------:|
| `nlohmann::json` | JSON for Modern C++ | [Github](https://github.com/nlohmann/json) |
| `Google Test` | C++ unit testing framework (git submodule) | [Github](https://github.com/google/googletest) |

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
        DirectoryWatcher/          Real-time file system monitoring (Windows overlapped I/O)
        HotReload/                 Versioned DLL hot-reload system
      ThirdParty/                Third-party libraries (nlohmann_json, googletest)
      Programs/
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
  Tests/
    Core.Math.Tests/             Core math unit tests (GoogleTest, 284+ tests)
    Core.Delegates.Tests/        Delegate system unit tests (GoogleTest, 37 tests)
    Core.Config.Tests/           Config system unit tests (GoogleTest)
    Core.ThreadPool.Tests/       Thread pool unit tests (GoogleTest)
    Core.TaskGraph.Tests/        Task graph unit tests (GoogleTest)
    ApplicationCore.Tests/       Window & message pump tests (GoogleTest, 20 tests)
    RenderCore.Tests/            RenderCore module tests (GoogleTest)
    AsciiRenderer.Tests/         AsciiRenderer module tests (GoogleTest)
    Engine.Tests/                Engine module tests (GoogleTest)
    Engine.TickSystem.Tests/     Tick system unit tests (GoogleTest)
    EnhancedInput.Tests/         Enhanced Input system tests (GoogleTest)
    Core.Ticker.Tests/           FTSTicker unit tests (GoogleTest, 11 tests)
    DirectoryWatcher.Tests/      DirectoryWatcher module tests (GoogleTest, 7 tests)
    HotReload.Tests/             HotReload module tests (GoogleTest, 7 tests)
    DllExportMacroTest/          DLL export macro validation
    ModuleInterfaceTest/         Module interface integration test
    ModuleManagerTest/           Module manager integration test
    LaunchModuleTest/            Launch module integration test
    NlohmannJsonTest/            nlohmann_json integration test
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
