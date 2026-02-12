<p align="center"><img src="https://github.com/user-attachments/assets/12663581-fb81-4364-96d6-36e57d6cfd4f" alt="Logo" width="300"></p>

<h1 align="center"> Enigma Engine </h1>
<h4 align="center">面向体素游戏开发的现代化、渐进式、轻量级模块化游戏引擎</h4>
<p align="center">
<a href="https://www.codefactor.io/repository/github/caishangqi/EnigmaEngine"><img src="https://www.codefactor.io/repository/github/caishangqi/EnigmaEngine/badge" alt="CodeFactor" /></a>
<img alt="Renderer Backend" src="https://img.shields.io/badge/Render API-ASCII-242629">
<img alt="C++ Language Level" src="https://img.shields.io/badge/C++-23-cherry">
<img alt="Dot Net" src="https://img.shields.io/badge/.Net-8.0-573ccf">
<img alt="GitHub branch checks state" src="https://img.shields.io/github/checks-status/Caishangqi/EnigmaEngine/master?label=build">
<img alt="GitHub code size in bytes" src="https://img.shields.io/github/languages/code-size/Caishangqi/EnigmaEngine">
</p>

<p align="center"><a href="README.md">English</a></p>

## 概述

Enigma Engine 是一款专为体素游戏开发设计的 C++ 游戏引擎。采用模块化架构，支持运行时 DLL 加载/卸载、插件系统和多配置构建。引擎提供完整的项目脚手架工具链，从项目创建到打包发布均可通过 BuildTool 命令行完成。

## 特性

- 模块化架构：引擎、游戏逻辑和插件均以独立 DLL 模块运行，支持动态加载和依赖管理
- 插件系统：通过 `.eplugin` 描述符定义插件，支持加载阶段控制，自动发现和依赖检查
- 项目脚手架：一键创建项目、模块、插件，自动生成模板代码和配置文件
- 多配置构建：Debug / DebugGame / Development / Shipping / Test 五种构建配置
- Visual Studio 集成：自动生成 `.sln` 和 `.vcxproj` 项目文件
- Unreal 风格 API：熟悉的命名约定和架构模式（ModuleRules、TargetRules、GameInstance）

## 计划中的特性

- 引入支持模块和插件 DLL 热重载的游戏编辑器
- 引入基于 ASCII 的 720p 渲染器
- 将 `create-module`、`create-plugin` 等构建操作集成到游戏编辑器中

## 构建

### 环境要求

- C++23 编译器（MSVC 17 2022 或更高版本）
- CMake 3.20+
- .NET 8.0 SDK（用于 BuildTool）

### BuildTool

BuildTool 是基于 C# .NET 8 的命令行构建工具，负责项目扫描、依赖解析、CMake 生成、编译和打包。

```bash
# 构建项目（默认 Development 配置）
BuildTool build <project-path>

# 指定构建配置
BuildTool build <project-path> -c DebugGame
BuildTool build <project-path> -c Shipping

# 清理 / 重建
BuildTool clean <project-path>
BuildTool rebuild <project-path>

# 生成 Visual Studio 解决方案
BuildTool generate-project-files <project-path>

# 打包发布（自动使用 Shipping 配置）
BuildTool package <project-path> -o <output-path>
```

### 项目脚手架

```bash
# 创建新游戏项目
BuildTool create-project --name MyGame --location ./Games

# 创建游戏模块
BuildTool create-module <project-path> --name GameUtils

# 创建插件
BuildTool create-plugin <project-path> --name MyFeature --category Gameplay

# 移除模块（自动检查依赖）
BuildTool remove-module <project-path> --name GameUtils

# 移除插件
BuildTool remove-plugin <project-path> --name MyFeature
```

### 构建配置

| 配置 | 说明 | 链接方式 | 优化级别 |
|------|:----:|:--------:|:--------:|
| `Debug` | 完整调试，无优化 | 模块化 (DLL) | /Od |
| `DebugGame` | 引擎优化，游戏可调试 | 模块化 (DLL) | /O1 |
| `Development` | 开发构建，中等优化 | 模块化 (DLL) | /O1 |
| `Shipping` | 发布构建，完全优化 | 单体 (静态链接) | /O2 |
| `Test` | 自动化测试构建 | 模块化 (DLL) | /O2 |

## 模块

| **名称** | **说明** | **状态** |
|----------|:--------:|:--------:|
| `Enigma::Core` | 基础模块，提供模块系统、日志、断言、平台抽象层（HAL），零外部依赖 | stable |
| `Enigma::Engine` | 引擎核心，提供 FEngineLoop 引擎循环、FGameEngine、FGameInstance 游戏实例和模块加载阶段管理 | stable |
| `Enigma::Launch` | 入口点模块，提供 GuardedMain 守护主函数和平台特定启动逻辑（main / WinMain） | stable |

## 第三方库

| **名称** | **说明** | **链接** |
|----------|:--------:|:--------:|
| `nlohmann::json` | JSON for Modern C++ | [Github](https://github.com/nlohmann/json) |

## 项目结构

```
EnigmaEngine/
  Engine/
    Source/Runtime/         引擎运行时模块 (Core, Engine, Launch)
    Source/ThirdParty/      第三方库 (nlohmann_json)
    Templates/              项目/模块/插件代码模板
  BuildTool/                C# .NET 构建工具
  Games/
    EnigmaArcade/           示例游戏项目
      Source/               游戏模块源码
      Plugins/              游戏插件
  Tests/                    分阶段集成测试项目
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
<h4 align="center">在 <a href="https://github.com/Caishangqi">SMU Pages</a> 了解更多关于 EnigmaEngine 的信息</h4>
<h4 align="center">需要定制支持？<a href="https://github.com/Caishangqi">点击这里</a></h4>
