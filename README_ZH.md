<p align="center"><img src="https://github.com/user-attachments/assets/12663581-fb81-4364-96d6-36e57d6cfd4f" alt="Logo" width="300"></p>

<h1 align="center"> Enigma Engine </h1>
<h4 align="center">面向体素游戏开发的现代化、渐进式、轻量级模块化游戏引擎</h4>
<p align="center">
<a href="https://www.codefactor.io/repository/github/caishangqi/EnigmaEngine"><img src="https://www.codefactor.io/repository/github/caishangqi/EnigmaEngine/badge" alt="CodeFactor" /></a>
<img alt="Renderer Backend" src="https://img.shields.io/badge/Render API-ASCII-242629">
<img alt="C++ Language Level" src="https://img.shields.io/badge/C++-26-cherry">
<img alt="Dot Net" src="https://img.shields.io/badge/.Net-9.0-573ccf">
<img alt="GitHub branch checks state" src="https://img.shields.io/github/checks-status/Caishangqi/EnigmaEngine/master?label=build">
<img alt="GitHub code size in bytes" src="https://img.shields.io/github/languages/code-size/Caishangqi/EnigmaEngine">
</p>

<p align="center"><a href="README.md">English</a></p>

## 概述

Enigma Engine 是一款专为体素游戏开发设计的 C++ 游戏引擎。采用模块化架构，支持运行时 DLL 加载/卸载、插件系统和多配置构建。引擎提供完整的项目脚手架工具链，从项目创建到打包发布均可通过 BuildTool 命令行完成。

## 特性

- 模块化架构：引擎、游戏逻辑和插件均以独立 DLL 模块运行，基于清单的 DLL 加载实现多配置隔离，引擎插件自动发现，支持依赖管理
- 结构化日志：基于分类的 `ENIGMA_LOG` 宏，编译期详细度门控（Shipping 构建零开销），运行时级别控制，自动 stderr/stdout 路由
- 插件系统：通过 `.eplugin` 描述符定义插件，支持加载阶段控制，自动发现和依赖检查
- INI 配置系统：UE 风格分层配置，4 层合并（引擎基础 → 插件 → 项目默认 → 用户本地），类型化读写、数组操作符、插件双轨支持
- 项目脚手架：一键创建项目、模块、插件，自动生成模板代码和配置文件
- 多配置构建：Debug / DebugGame / Development / Shipping / Test 五种构建配置
- Visual Studio 集成：自动生成 `.sln` 和 `.vcxproj` 项目文件
- Unreal 风格 API：熟悉的命名约定和架构模式（ModuleRules、TargetRules、GameInstance）
- 核心数学库：FVector、FMatrix、FQuat、FRotator、FTransform 等完整 3D 数学类型，右手 Y-up 坐标系，constexpr 友好
- 委托与事件系统：类型安全的 TDelegate、TMulticastDelegate，FDelegateHandle 生命周期管理，支持静态/Lambda/成员函数绑定
- 引擎子系统框架：可扩展的 SubsystemCollection，自动生命周期管理，类似 UE 的子系统架构
- 异步任务基础设施：FThreadPool 通用线程池、FTaskGraph 基于依赖的并行任务调度，支持命名任务和前置依赖链
- Tick 系统：FTickTaskManager 子系统，支持 Tick 组（PreUpdate/Update/PostUpdate）、前置依赖排序、可选多线程调度（FTaskGraph）和可配置 Tick 间隔
- 游戏对象框架：FScene / FGameObject / FComponent 层级结构，生命周期钩子（OnAttach、BeginPlay、Update、OnDetach），遵循 UE5 模式的场景驱动 BeginPlay 分发，支持动态对象
- 增强输入系统：基于动作的输入系统，支持触发器（Pressed/Released/Down）、修饰器（Negate/Swizzle/DeadZone/Scalar）和带优先级的映射上下文
- ASCII 渲染器：基于帧缓冲的 ASCII 字符渲染，Z 深度排序、场景视图摄像机、Y-up 坐标约定、VT100 终端输出

## 计划中的特性

- 引入支持模块和插件 DLL 热重载的游戏编辑器
- 将 `create-module`、`create-plugin` 等构建操作集成到游戏编辑器中
- 通过 Viewport 抽象层将渲染器初始化与窗口创建解耦（类似 UE 的 RHI / GameViewport 分离），以支持多渲染后端（DX12、Vulkan）

## 构建

### 环境要求

- C++26 编译器（MSVC 17 2022 或更高版本）
- CMake 3.20+
- .NET 9.0 SDK（用于 BuildTool）

### BuildTool

BuildTool 是基于 C# .NET 9 的命令行构建工具，负责项目扫描、依赖解析、CMake 生成、编译和打包。

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
BuildTool create-project --name MyGame --location .

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

| 配置 | 说明 | 链接方式 | 优化级别 | EXE 位置 | 引擎 DLL | 游戏 DLL |
|------|:----:|:--------:|:--------:|:--------:|:--------:|:--------:|
| `Debug` | 完整调试，无优化 | 模块化 (DLL) | /Od | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |
| `DebugGame` | 引擎优化，游戏可调试 | 模块化 (DLL) | /O1 | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |
| `Development` | 开发构建，中等优化 | 模块化 (DLL) | /O1 | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |
| `Shipping` | 发布构建，完全优化 | 单体 (静态链接) | /O2 | Project/Binaries/ | N/A | N/A |
| `Test` | 自动化测试构建 | 模块化 (DLL) | /O2 | Engine/Binaries/ | Engine/Binaries/ | Project/Binaries/ |

### 二进制输出布局

模块化构建（Debug / DebugGame / Development）遵循 UE 的分离布局：

```
Engine/Binaries/Win64/           EXE + 引擎模块 DLL + .modules 清单
{Project}/Binaries/Win64/        游戏模块 DLL + .modules + .target 清单
{Project}/Plugins/{Name}/Binaries/Win64/   插件 DLL + .modules 清单
```

Shipping 构建生成单一的单体可执行文件：

```
{Project}/Binaries/Win64/        单体 EXE + .target 清单
```

EXE 通过 `--project-dir=` 命令行参数在运行时定位游戏 DLL（VS 调试器设置中已自动配置）。

## 模块

| **名称** | **说明** | **状态** |
|----------|:--------:|:--------:|
| `Enigma::Core` | 基础模块，提供模块系统、日志、断言、平台抽象层（HAL）、委托/事件系统（TDelegate、TMulticastDelegate）、INI 配置系统（FConfigCacheIni、GConfig）、核心数学类型（FVector、FMatrix、FQuat、FRotator、FTransform）和异步任务基础设施（FThreadPool、FTaskGraph） | stable |
| `Enigma::ApplicationCore` | 平台无关的应用程序和窗口抽象（FGenericApplication、FGenericWindow、FGenericApplicationMessageHandler），含 Win32 实现 | stable |
| `Enigma::RenderCore` | 渲染器接口抽象层（IRendererModule），将引擎与具体渲染器实现解耦 | stable |
| `Enigma::AsciiRenderer` | ASCII 字符渲染器，帧缓冲、Z 深度排序、场景视图摄像机、VT100 终端输出 | stable |
| `Enigma::Engine` | 引擎核心，提供 FEngineLoop 引擎循环、FGameEngine 配置驱动窗口创建、FGameInstance 游戏实例、SubsystemCollection 子系统集合、FTickTaskManager Tick 调度、FScene/FGameObject/FComponent 游戏对象框架（场景驱动 BeginPlay 生命周期）和模块加载阶段管理 | stable |
| `Enigma::Launch` | 入口点模块，提供 GuardedMain 守护主函数和平台特定启动逻辑（main / WinMain） | stable |
| `Enigma::EnhancedInput` | 基于动作的输入系统，支持触发器、修饰器和映射上下文（引擎插件） | stable |

## 第三方库

| **名称** | **说明** | **链接** |
|----------|:--------:|:--------:|
| `nlohmann::json` | JSON for Modern C++ | [Github](https://github.com/nlohmann/json) |
| `Google Test` | C++ 单元测试框架 (git submodule) | [Github](https://github.com/google/googletest) |

## 项目结构

```
EnigmaEngine/
  Engine/
    Binaries/Win64/              引擎 DLL + EXE（模块化构建）
    Config/                      引擎基础配置 (BaseEngine.ini, BaseGame.ini)
    Intermediate/                引擎构建中间文件 + 生成的 .vcxproj 文件
    Source/
      Runtime/                   引擎运行时模块
        Core/                      基础模块：模块系统、日志、数学库、委托、配置、异步任务
        ApplicationCore/           平台应用程序与窗口抽象 (Win32)
        RenderCore/                渲染器接口抽象层 (IRendererModule)
        AsciiRenderer/             ASCII 帧缓冲渲染器，Z 深度排序、场景视图摄像机
        Engine/                    引擎循环、GameEngine、GameInstance、SubsystemCollection、TickSystem、Scene/GameObject/Component
        Launch/                    入口点 (GuardedMain, main/WinMain)
      ThirdParty/                第三方库 (nlohmann_json, googletest)
      Programs/
        BuildTool/               C# .NET 9 命令行构建工具
    Plugins/
      EnhancedInput/             引擎插件：基于动作的输入系统
        EnhancedInput.eplugin      插件描述符
        Source/EnhancedInput/      模块源码 (Public/ + Private/)
        Binaries/                  插件 DLL（模块化构建）
    Templates/                   脚手架模板（项目、模块、插件）
  EnigmaArcade/                  示例游戏项目
    EnigmaArcade.eproject        项目描述符
    Config/                      项目配置 (DefaultEngine.ini, DefaultGame.ini)
    Source/
      EnigmaArcade/              主游戏模块
    Plugins/
      ArcadeFeature/             游戏插件
        ArcadeFeature.eplugin      插件描述符
        Config/                    插件配置 (DefaultArcadeFeature.ini)
        Source/ArcadeFeature/      模块源码 (Public/ + Private/)
    Binaries/Win64/              游戏 DLL（模块化）或单体 EXE（Shipping）
    Intermediate/                构建中间文件 + 生成的 .vcxproj 文件
  Tests/
    Core.Math.Tests/             Core 数学库单元测试 (GoogleTest, 284+ 个测试)
    Core.Delegates.Tests/        委托系统单元测试 (GoogleTest, 37 个测试)
    Core.Config.Tests/           配置系统单元测试 (GoogleTest)
    Core.ThreadPool.Tests/       线程池单元测试 (GoogleTest)
    Core.TaskGraph.Tests/        任务图单元测试 (GoogleTest)
    ApplicationCore.Tests/       窗口与消息泵测试 (GoogleTest, 20 个测试)
    RenderCore.Tests/            RenderCore 模块测试 (GoogleTest)
    AsciiRenderer.Tests/         AsciiRenderer 模块测试 (GoogleTest)
    Engine.Tests/                Engine 模块测试 (GoogleTest)
    Engine.TickSystem.Tests/     Tick 系统单元测试 (GoogleTest)
    EnhancedInput.Tests/         增强输入系统测试 (GoogleTest)
    DllExportMacroTest/          DLL 导出宏验证
    ModuleInterfaceTest/         模块接口集成测试
    ModuleManagerTest/           模块管理器集成测试
    LaunchModuleTest/            Launch 模块集成测试
    NlohmannJsonTest/            nlohmann_json 集成测试
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
