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

- Modular architecture: engine, game logic, and plugins run as independent DLL modules with dynamic loading and dependency management
- Plugin system: plugins defined via `.eplugin` descriptors with loading phase control, auto-discovery, and dependency checking
- INI config system: UE-style layered config with 4-layer merging (Engine Base → Plugin → Project Default → User), typed getters/setters, array operators, plugin dual-track support
- Project scaffolding: one-command creation of projects, modules, and plugins with auto-generated template code and configuration
- Multi-configuration builds: Debug / DebugGame / Development / Shipping / Test
- Visual Studio integration: auto-generated `.sln` and `.vcxproj` project files
- Unreal-style API: familiar naming conventions and architectural patterns (ModuleRules, TargetRules, GameInstance)
- Core math library: FVector, FMatrix, FQuat, FRotator, FTransform and more, right-hand Y-up coordinate system, constexpr-friendly
- Delegate & event system: type-safe TDelegate, TMulticastDelegate with FDelegateHandle lifecycle management, supports static/lambda/member function bindings
- ASCII renderer: frame-buffer based ASCII art rendering with Z-depth sorting, scene view camera, and VT100 terminal output

## Planned Features

- Game Editor with hot-reload support for game module and plugin DLLs
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
| `Enigma::Core` | Foundation module providing the module system, logging, assertions, HAL platform abstraction, delegate/event system (TDelegate, TMulticastDelegate), INI config system (FConfigCacheIni, GConfig), and core math types (FVector, FMatrix, FQuat, FRotator, FTransform) | stable |
| `Enigma::ApplicationCore` | Platform-agnostic application and window abstraction (FGenericApplication, FGenericWindow, FGenericApplicationMessageHandler) with Win32 implementation | stable |
| `Enigma::RenderCore` | Renderer interface abstraction layer (IRendererModule) decoupling engine from concrete renderer implementations | stable |
| `Enigma::AsciiRenderer` | ASCII art renderer with frame-buffer, Z-depth sorting, scene view camera, and VT100 terminal output | stable |
| `Enigma::Engine` | Engine core providing FEngineLoop, FGameEngine with config-driven window creation, FGameInstance, and module loading phase management | stable |
| `Enigma::Launch` | Entry point module providing GuardedMain and platform-specific launch logic (main / WinMain) | stable |

## Third Party

| **Name** | **Description** | **Link** |
|----------|:---------------:|:--------:|
| `nlohmann::json` | JSON for Modern C++ | [Github](https://github.com/nlohmann/json) |
| `Google Test` | C++ unit testing framework (git submodule) | [Github](https://github.com/google/googletest) |

## Project Structure

```
EnigmaEngine/
  Engine/
    Binaries/Win64/         Engine DLLs + EXE (Modular builds)
    Config/                 Engine base config (BaseEngine.ini, BaseGame.ini)
    Intermediate/           Engine build intermediates + generated .vcxproj files
    Source/Runtime/         Runtime modules (Core, ApplicationCore, RenderCore, AsciiRenderer, Engine, Launch)
    Source/ThirdParty/      Third-party libraries (nlohmann_json, googletest)
    Source/Programs/        Build tools (BuildTool)
    Templates/              Project / Module / Plugin code templates
  EnigmaArcade/             Example game project
    Binaries/Win64/         Game DLLs (Modular) or monolithic EXE (Shipping)
    Config/                 Project config (DefaultEngine.ini, DefaultGame.ini)
    Intermediate/           Project build intermediates + generated .vcxproj files
    Source/                 Game module sources
    Plugins/                Game plugins (each with own Binaries/ and Intermediate/)
  Tests/
    Core.Math.Tests/        Core math unit tests (GoogleTest, 284 tests)
    Core.Delegates.Tests/   Delegate system unit tests (GoogleTest, 37 tests)
    Core.Config.Tests/      Config system unit tests (GoogleTest)
    ApplicationCore.Tests/  ApplicationCore integration tests (GoogleTest, 20 tests)
    RenderCore.Tests/       RenderCore module tests (GoogleTest)
    AsciiRenderer.Tests/    AsciiRenderer module tests (GoogleTest)
    Engine.Tests/           Engine module tests (GoogleTest)
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
